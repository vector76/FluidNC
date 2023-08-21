// Copyright (c) 2021 -	Mitch Bradley
// Use of this source code is governed by a GPLv3 license that can be found in the LICENSE file.

#include "Channel.h"
#include "Report.h"                 // report_gcode_modes
#include "Machine/MachineConfig.h"  // config
#include "Serial.h"                 // execute_realtime_command
#include "Limits.h"
#include "Settings.h"  // for Command

void Channel::flushRx() {
    _linelen   = 0;
    _lastWasCR = false;
    while (_queue.size()) {
        _queue.pop();
    }
}

bool findDollarKeyValue(char* line, char** keystart_p, int* keylength_p, char** valstart_p) {
    char* keystart  = line + 1;
    int   keylength = 0;
    char* valstart  = strchr(line, '=');

    if (line[0] == '$') {
        // Advance key start over all non-whitespace
        while (isspace(*keystart) && *keystart != '\0' && *keystart != '=') {
            keystart++;
        }
        while (keystart[keylength] != '\0' && keystart[keylength] != '=' && !isspace(keystart[keylength])) {
            keylength++;  // keep advancing until end or whitespace or '='
        }
        // may break early if keystart was already '\0' and it just produces zero length key

        if (valstart != nullptr) {
            valstart++;
        }

        *keystart_p  = keystart;
        *keylength_p = keylength;
        *valstart_p  = valstart;
        return true;
    }

    return false;
}

void Channel::tryProcess() {
    // whenever buildLine transitions to _isComplete == true, this checks if the command can be dispatched
    _line[_linelen] = '\0';
    char* key;
    int   keylength;
    char* value;
    if (findDollarKeyValue(_line, &key, &keylength, &value)) {
        char tmp_key_end = key[keylength];
        key[keylength]   = '\0';
        Command* cmd     = Command::findCommand(key);
        if (cmd && (cmd->getPermissions() == WG_CH || (value == nullptr && cmd->getPermissions() == WU_CH))) {
            // channel will handle the command (more precisely, the polling thread handles it)
            //log_info_to(*this, "Command " << key << " intercepted by buildLine/tryProcess");
            ack(cmd->action(value, WebUI::AuthenticationLevel::LEVEL_GUEST, *this));
            _linelen    = 0;
            _isComplete = false;
        } else {
            key[keylength] = tmp_key_end;  // restore line to original form for handoff to other thread
            //log_info_to(*this, "Command '" << _line << "' not processed by buildLine/tryProcess");
        }
    } else if (_line[0] == '\0') {
        // Does empty command occur sometimes?
        //log_info_to(*this, "Empty command intercepted by buildLine/tryProcess");
        _linelen    = 0;
        _isComplete = false;
        // flush it out so it doesn't block other channel commands
    }
}

bool Channel::buildLine(char ch) {
    // returns true if ch was accepted, false if rejected (e.g. if line complete and not accepting more characters)
    if (_isComplete) {
        return false;
    }
    if (ch == '\n') {
        // if previous processing left cr at end, then consume lf with no effect
        if (_lastWasCR) {
            _lastWasCR = false;
            return true;
        }

        _isComplete = true;  // _line up to _linelen is fully built up and ready for copying
        tryProcess();
        return true;
    }
    _lastWasCR = ch == '\r';
    if (_lastWasCR) {
        _isComplete = true;  // _line up to _linelen is fully built up and ready for copying
        tryProcess();
        return true;
    }
    if (ch == '\b') {
        // Simple editing for interactive input - backspace erases
        if (_linelen) {
            --_linelen;
        }
        return true;
    }
    if (_linelen < (Channel::maxLine - 1)) {
        _line[_linelen++] = ch;
    } else {
        // overflow
    }
    return true;
}

uint32_t Channel::setReportInterval(uint32_t ms) {
    uint32_t actual = ms;
    if (actual) {
        actual = std::max(actual, uint32_t(50));
    }
    _reportInterval = actual;
    _nextReportTime = int32_t(xTaskGetTickCount());
    _lastTool       = 255;  // Force GCodeState report
    return actual;
}
static bool motionState() {
    return sys.state == State::Cycle || sys.state == State::Homing || sys.state == State::Jog;
}

void Channel::autoReportGCodeState() {
    // When moving, we suppress $G reports in which the only change is the motion mode
    // (e.g. G0/G1/G2/G3 changes) because rapid-fire motion mode changes are fairly common.
    // We would rather not issue a $G report after every GCode line.
    // Similarly, F and S values can change rapidly, especially in laser programs.
    // F and S values are also reported in ? status reports, so they will show up
    // at the chosen periodic rate there.
    if (motionState()) {
        // Force the compare to succeed if the only change is the motion mode
        _lastModal.motion = gc_state.modal.motion;
    }
    if (memcmp(&_lastModal, &gc_state.modal, sizeof(_lastModal)) || _lastTool != gc_state.tool ||
        (!motionState() && (_lastSpindleSpeed != gc_state.spindle_speed || _lastFeedRate != gc_state.feed_rate))) {
        report_gcode_modes(*this);
        memcpy(&_lastModal, &gc_state.modal, sizeof(_lastModal));
        _lastTool         = gc_state.tool;
        _lastSpindleSpeed = gc_state.spindle_speed;
        _lastFeedRate     = gc_state.feed_rate;
    }
}
void Channel::autoReport() {
    if (_reportInterval) {
        auto limitState = limits_get_state();
        auto probeState = config->_probe->get_state();
        if (_reportWco || sys.state != _lastState || limitState != _lastLimits || probeState != _lastProbe ||
            (motionState() && (int32_t(xTaskGetTickCount()) - _nextReportTime) >= 0)) {
            if (_reportWco) {
                report_wco_counter = 0;
            }
            _reportWco  = false;
            _lastState  = sys.state;
            _lastLimits = limitState;
            _lastProbe  = probeState;

            _nextReportTime = xTaskGetTickCount() + _reportInterval;
            report_realtime_status(*this);
        }
        if (_reportNgc != CoordIndex::End) {
            report_ngc_coord(_reportNgc, *this);
            _reportNgc = CoordIndex::End;
        }
        autoReportGCodeState();
    }
}

Channel* Channel::pollLine(char* line) {
    handle();
    // Splitting into distinct cases makes it easier to reason about
    if (line) {
        // Possible we are already complete but command was not a channel-processable command
        while (1) {
            if (_isComplete) {
                _line[_linelen] = '\0';
                strcpy(line, _line);
                _linelen    = 0;
                _isComplete = false;
                return this;
            }

            int ch;
            // pull from queue first, then read after queue depleted
            if (_queue.size()) {
                ch = _queue.front();
                _queue.pop();
            } else {
                ch = read();
            }

            if (ch < 0) {
                break;
            }
            if (realtimeOkay(ch) && is_realtime_command(ch)) {
                execute_realtime_command(static_cast<Cmd>(ch), *this);
            } else {
                if (!buildLine(ch)) {
                    // internal error, shouldn't happen?
                    log_warn_to(*this, "buildLine couldn't store character");
                }
            }
        }
    } else {
        while (1) {
            int ch = read();

            if (ch < 0) {
                break;
            }
            // act on it now as realtime command, or save into _queue for later
            if (realtimeOkay(ch) && is_realtime_command(ch)) {
                execute_realtime_command(static_cast<Cmd>(ch), *this);
            } else {
                if (!buildLine(ch)) {
                    // failure means we were already complete and need to store to queue
                    _queue.push(uint8_t(ch));
                }
                // no need to check here for _isComplete because buildLine/tryProcess will have already tried to handle it
            }
        }
    }
    autoReport();
    return nullptr;
}

void Channel::ack(Error status) {
    if (status == Error::Ok) {
        log_to(*this, "ok");
        return;
    }
    // With verbose errors, the message text is displayed instead of the number.
    // Grbl 0.9 used to display the text, while Grbl 1.1 switched to the number.
    // Many senders support both formats.
    LogStream msg(*this, "error:");
    if (config->_verboseErrors) {
        msg << errorString(status);
    } else {
        msg << static_cast<int>(status);
    }
}
