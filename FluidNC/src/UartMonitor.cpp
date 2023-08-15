#include "UartMonitor.h"
#include "Machine/MachineConfig.h"
#include "TicToc.h"


void uart_write_str(Uart *dbuart, const char *s) {
    dbuart->write((uint8_t *)s, strlen(s));
}

char heap_char() {
    uint8_t kfree = xPortGetFreeHeapSize()/1024;
    if (kfree <= 9) {
        return '0' + kfree;  // 0 to 9
    }
    if (kfree <= 35) {
        return 'A'+(kfree-10);  // 10 to 35
    }
    return 'a' + (kfree-35);  // 36=a and higher (up to 61=z)
}

Channel * first_websocket() {
    int nch = allChannels._channelq.size();
    for (int i=0; i < nch; i++) {
        if (allChannels._channelq[i]->name()[0] == 'w') {
            return allChannels._channelq[i];
        }
    }
    return nullptr;
}

bool heartbeat_ram = true;
const char *heartbeat_message = nullptr;

bool heartbeat_debug(Uart *dbuart, char c) {
    char tmp[60];
    uint32_t heapvals[10];
    int32_t tickvals[11];

    if (c == 'a') {
        uart_write_str(dbuart, "\r\ngot letter 'a'\r\n");
    }
    else if (c == 'o') {
        //sprintf(tmp, "\r\noutput stuck line: %d\r\n", output_loop_stuck_at);
        //uart_write_str(dbuart, tmp);
        //sprintf(tmp, "output stuck channel: %s\r\n", output_stuck_on == nullptr ? "none" : output_stuck_on->name());
        //uart_write_str(dbuart, tmp);
    }
    else if (c == 't') {
        //int32_t main_us = toc_us(main_loop_last_iter);
        //int32_t poll_us = toc_us(polling_loop_last_iter);
        //int32_t out_us = toc_us(output_loop_last_iter);

        //sprintf(tmp, "\r\nmain loop time: %d.%03d ms, count: %u\r\n", main_us/1000, main_us%1000, main_loop_count);
        //uart_write_str(dbuart, tmp);
        //sprintf(tmp, "poll loop time: %d.%03d ms, count: %u\r\n", poll_us/1000, poll_us%1000, polling_loop_count);
        //uart_write_str(dbuart, tmp);
        //sprintf(tmp, "out loop time: %d.%03d ms, count: %u\r\n", out_us/1000, out_us%1000, output_loop_count);
        //uart_write_str(dbuart, tmp);
        //if (stuck_in_autoreport) {
        //    sprintf(tmp, "stuck in autoreport at %d\r\n", stuck_in_autoreport);
        //    uart_write_str(dbuart, tmp);
        //    sprintf(tmp, "stuck in report_realtime_status at line %d\r\n", report_stuck_at);
        //    uart_write_str(dbuart, tmp);
        //}
        //else {
        //    uart_write_str(dbuart, "not stuck in autoreport\r\n");
        //}
    }
    else if (c == 'h') {
        uint32_t currentFree = xPortGetFreeHeapSize();
        sprintf(tmp, "\r\nCurrent heap free: %u (%c)\r\n", currentFree, heap_char());
        uart_write_str(dbuart, tmp);
    }
    else if (c == 'm') {
        if (AllChannels::_mutex_general.try_lock()) {
            AllChannels::_mutex_general.unlock();
            uart_write_str(dbuart, "\r\nAllChannels::_mutex_general is not locked\r\n");
        }
        //else {
            //sprintf(tmp, "\r\nAllChannels::_mutex1 is locked by '%s' in '%s'", taskname, lockfun);
            //uart_write_str(dbuart, tmp);
            //sprintf(tmp, " (channel '%s')\r\n", channame);
            //uart_write_str(dbuart, tmp);
        //}
        if (AllChannels::_mutex_pollLine.try_lock()) {
            AllChannels::_mutex_pollLine.unlock();
            uart_write_str(dbuart, "AllChannels::_mutex2 is not locked\r\n");
        }
        //else {
        //    sprintf(tmp, "AllChannels::_mutex2 is locked by '%s' in '%s'", taskname, lockfun);
        //    uart_write_str(dbuart, tmp);
        //    sprintf(tmp, " (channel '%s')\r\n", channame);
        //    uart_write_str(dbuart, tmp);
        //}
    }
    else if (c == 'r') {
        heapLowWater = UINT_MAX;
        //heapLowWater2 = UINT_MAX;
        //max_message_queue_depth = 0;
        //msgdropped = false;
        uart_write_str(dbuart, "\r\nReset memory low watermark\r\n");
    }
    else if (c == 'd') {
        //sprintf(tmp, "\r\nMax message depth: %d\r\n", max_message_queue_depth);
        //uart_write_str(dbuart, tmp);
        //sprintf(tmp, "%sessages dropped\r\n", msgdropped ? "M" : "No m");
        //uart_write_str(dbuart, tmp);
    }
    else if (c == 'f') {
        //freeze_io_tasks = !freeze_io_tasks;
        //sprintf(tmp, "\r\nI/O tasks %sfrozen\r\n", freeze_io_tasks ? "" : "un");
        //uart_write_str(dbuart, tmp);
    }
    else if (c == 'F') {
        //freeze_main_task = !freeze_main_task;
        //sprintf(tmp, "\r\nMain task %sfrozen\r\n", freeze_main_task ? "" : "un");
        //uart_write_str(dbuart, tmp);
    }
    else if (c == '1') {
        // heap allocation test
        Channel *ch0 = allChannels._channelq[0];
        std::string linestr("Here is a string");
        for (int i=0; i < 10; i++) {
            // this doesn't use any heap at all
            //std::string* line = &linestr;
            //LogMessage msg { ch0, (void *)line, true };
            //std::string* s = static_cast<std::string*>(msg.line);
            //msg.channel->println(s->c_str());

            send_line(*ch0, linestr);

            heapvals[i] = xPortGetFreeHeapSize();
        }
        for (int i=0; i < 10; i++) {
            sprintf(tmp, "Current heap free: %u (%c)\r\n", heapvals[i], heap_char());
            uart_write_str(dbuart, tmp);
        }
        uart_write_str(dbuart, "That was simple `send_line` ten times to first channel (uart 0)\r\n");
    }
    else if (c == '2') {
        // heap allocation test
        Channel *ch0 = &allChannels;
        std::string linestr("Here is a string");
        for (int i=0; i < 10; i++) {
            // this doesn't use any heap at all
            //std::string* line = &linestr;
            //LogMessage msg { ch0, (void *)line, true };
            //std::string* s = static_cast<std::string*>(msg.line);
            //msg.channel->println(s->c_str());

            send_line(*ch0, linestr);
            
            heapvals[i] = xPortGetFreeHeapSize();
        }
        for (int i=0; i < 10; i++) {
            sprintf(tmp, "Current heap free: %u (%c)\r\n", heapvals[i], heap_char());
            uart_write_str(dbuart, tmp);
        }
        uart_write_str(dbuart, "That was simple `send_line` ten times to allChannels\r\n");
    }
    else if (c == '3') {
        for (int i=0; i < 10; i++) {
            log_debug("Here is a string " << i);

            heapvals[i] = xPortGetFreeHeapSize();
        }
        for (int i=0; i < 10; i++) {
            sprintf(tmp, "Current heap free: %u (%c)\r\n", heapvals[i], heap_char());
            uart_write_str(dbuart, tmp);
        }
        uart_write_str(dbuart, "That was `log_debug` ten times with no qualifiers (all channels)\r\n");
    }
    else if (c == '4') {
        for (int i=0; i < 10; i++) {
            log_info("Here is a string " << i);

            heapvals[i] = xPortGetFreeHeapSize();
        }
        for (int i=0; i < 10; i++) {
            sprintf(tmp, "Current heap free: %u (%c)\r\n", heapvals[i], heap_char());
            uart_write_str(dbuart, tmp);
        }
        uart_write_str(dbuart, "That was `log_info` ten times with no qualifiers (all channels)\r\n");
    }
    else if (c == '5') {
        void *ptrs[10];
        for (int i=0; i < 10; i++) {
            ptrs[i] = malloc(50);

            heapvals[i] = xPortGetFreeHeapSize();
        }
        for (int i=0; i < 10; i++) {
            sprintf(tmp, "Current heap free: %u (%c)\r\n", heapvals[i], heap_char());
            uart_write_str(dbuart, tmp);
            free(ptrs[i]);
        }
        uart_write_str(dbuart, "That was `malloc(50)` ten times\r\n");
    }
    else if (c == '6') {
        Channel *wsch = first_websocket();
        if (wsch != nullptr) {
            tickvals[0] = tic();
            for (int i=0; i < 10; i++) {
                sprintf(tmp, "Current heap free %d: %u (%c)\r\n", i, xPortGetFreeHeapSize(), heap_char());
                wsch->write(tmp, strlen(tmp));
                tickvals[i+1] = tic();
            }
            uart_write_str(dbuart, "That was `channel->write()` ten times to first websocket\r\n");
            int32_t mintime = INT32_MAX;
            int32_t maxtime = 0;
            for (int i=0; i < 10; i++) {
                if (tickvals[i+1]-tickvals[i] < mintime) {
                    mintime = tickvals[i+1]-tickvals[i];
                }
                if (tickvals[i+1]-tickvals[i] > maxtime) {
                    maxtime = tickvals[i+1]-tickvals[i];
                }
            }
            int32_t avgtime = (tickvals[10]-tickvals[0])/10;
            sprintf(tmp, "Fastest individual message: %d.%03d\r\n", (mintime/ticks_per_us)/1000, (mintime/ticks_per_us)%1000);
            uart_write_str(dbuart, tmp);
            sprintf(tmp, "Slowest individual message: %d.%03d\r\n", (maxtime/ticks_per_us)/1000, (maxtime/ticks_per_us)%1000);
            uart_write_str(dbuart, tmp);
            sprintf(tmp, "Average individual message: %d.%03d\r\n", (avgtime/ticks_per_us)/1000, (avgtime/ticks_per_us)%1000);
            uart_write_str(dbuart, tmp);
            for (int i=0; i < 10; i++) {
                if (i == 0) {
                    uart_write_str(dbuart, "Individual times: ");
                }
                if (i == 5) {
                    uart_write_str(dbuart, "\r\n");
                }
                int32_t time_us = (tickvals[i+1]-tickvals[i])/ticks_per_us;
                sprintf(tmp, "%d.%03d ", time_us/1000, time_us%1000);
                uart_write_str(dbuart, tmp);
            }
        }
    }
    else if (c == '7') {
        Channel *wsch = first_websocket();
        if (wsch != nullptr) {
            for (int i=0; i < 10; i++) {
                sprintf(tmp, "Current heap free %d: %u (%c)", i, xPortGetFreeHeapSize(), heap_char());
                log_info_to(*wsch, tmp);
            }
            uart_write_str(dbuart, "That was `log_info_to` ten times targeting first websocket\r\n");
        }
    }
    else if (c == '8') {
        Channel *ch0 = allChannels._channelq[0];
        if (ch0 != nullptr) {
            tickvals[0] = tic();
            for (int i=0; i < 10; i++) {
                sprintf(tmp, "Current heap free %d: %u (%c)\r\n", i, xPortGetFreeHeapSize(), heap_char());
                ch0->write(tmp, strlen(tmp));
                tickvals[i+1] = tic();
            }
            uart_write_str(dbuart, "That was `channel->write()` ten times to first channel (uart 0)\r\n");
            int32_t mintime = INT32_MAX;
            int32_t maxtime = 0;
            for (int i=0; i < 10; i++) {
                if (tickvals[i+1]-tickvals[i] < mintime) {
                    mintime = tickvals[i+1]-tickvals[i];
                }
                if (tickvals[i+1]-tickvals[i] > maxtime) {
                    maxtime = tickvals[i+1]-tickvals[i];
                }
            }
            int32_t avgtime = (tickvals[10]-tickvals[0])/10;
            sprintf(tmp, "Fastest individual message: %d.%03d\r\n", (mintime/ticks_per_us)/1000, (mintime/ticks_per_us)%1000);
            uart_write_str(dbuart, tmp);
            sprintf(tmp, "Slowest individual message: %d.%03d\r\n", (maxtime/ticks_per_us)/1000, (maxtime/ticks_per_us)%1000);
            uart_write_str(dbuart, tmp);
            sprintf(tmp, "Average individual message: %d.%03d\r\n", (avgtime/ticks_per_us)/1000, (avgtime/ticks_per_us)%1000);
            uart_write_str(dbuart, tmp);
            for (int i=0; i < 10; i++) {
                if (i == 0) {
                    uart_write_str(dbuart, "Individual times: ");
                }
                if (i == 5) {
                    uart_write_str(dbuart, "\r\n");
                }
                int32_t time_us = (tickvals[i+1]-tickvals[i])/ticks_per_us;
                sprintf(tmp, "%d.%03d ", time_us/1000, time_us%1000);
                uart_write_str(dbuart, tmp);
            }
        }
    }
    else if (c == '9') {
        Channel *ch0 = &allChannels;
        if (ch0 != nullptr) {
            tickvals[0] = tic();
            for (int i=0; i < 10; i++) {
                sprintf(tmp, "Current heap free %d: %u (%c)\r\n", i, xPortGetFreeHeapSize(), heap_char());
                ch0->write(tmp, strlen(tmp));
                tickvals[i+1] = tic();
            }
            uart_write_str(dbuart, "That was `channel->write()` ten times to allChannels\r\n");
            int32_t mintime = INT32_MAX;
            int32_t maxtime = 0;
            for (int i=0; i < 10; i++) {
                if (tickvals[i+1]-tickvals[i] < mintime) {
                    mintime = tickvals[i+1]-tickvals[i];
                }
                if (tickvals[i+1]-tickvals[i] > maxtime) {
                    maxtime = tickvals[i+1]-tickvals[i];
                }
            }
            int32_t avgtime = (tickvals[10]-tickvals[0])/10;
            sprintf(tmp, "Fastest individual message: %d.%03d\r\n", (mintime/ticks_per_us)/1000, (mintime/ticks_per_us)%1000);
            uart_write_str(dbuart, tmp);
            sprintf(tmp, "Slowest individual message: %d.%03d\r\n", (maxtime/ticks_per_us)/1000, (maxtime/ticks_per_us)%1000);
            uart_write_str(dbuart, tmp);
            sprintf(tmp, "Average individual message: %d.%03d\r\n", (avgtime/ticks_per_us)/1000, (avgtime/ticks_per_us)%1000);
            uart_write_str(dbuart, tmp);
            for (int i=0; i < 10; i++) {
                if (i == 0) {
                    uart_write_str(dbuart, "Individual times: ");
                }
                if (i == 5) {
                    uart_write_str(dbuart, "\r\n");
                }
                int32_t time_us = (tickvals[i+1]-tickvals[i])/ticks_per_us;
                sprintf(tmp, "%d.%03d ", time_us/1000, time_us%1000);
                uart_write_str(dbuart, tmp);
            }
        }
    }
    else {
        return false;  // did not match one of the specified characters
    }
    return true;  // matched one of the specified characters
}

//extern bool i2sout_interruption_report;

void hearbeat_loop(void *unused) {
    bool heartbeat = true;
    uint8_t hbspeed = 0;
    Uart *dbuart = config->_uarts[2];

    if (dbuart != nullptr) {
        uart_write_str(dbuart, "\r\n---- debug (o/t/m/h/x/r/q) ----\r\n");
        for (int loopct = 1; true; loopct++) {
            char c = '\0';
            int cycletime_ms = 500;
            if (hbspeed == 1) {
                cycletime_ms = 200;
            }
            if (hbspeed == 2) {
                // 1/60th of a second is 16.66666 ms
                // 1/3 of the time delay 16 ms, and 2/3 of the time delay 17 ms
                cycletime_ms = 16 + (((loopct % 3) == 0) ? 0 : 1);
            }
            int nread = dbuart->timedReadBytes(&c, 1, cycletime_ms);  // serves to set heartbeat cadence
            if (heartbeat_message != nullptr) {
                uart_write_str(dbuart, "\r\n");
                dbuart->write((uint8_t *)heartbeat_message, strlen(heartbeat_message));
                uart_write_str(dbuart, "\r\n");
            }
            else if (nread == 0) {
                if (heartbeat_ram) {
                    dbuart->write(heap_char());
                }
                else {
                    dbuart->write('.');
                }
                if (loopct % 60 == 0) {
                    uart_write_str(dbuart, "\r\n");
                }
            }
            else if (c == 'p') {
                hbspeed = (hbspeed+1) % 3;
            }
            else if (c == 'q') {
                heartbeat_ram = !heartbeat_ram;
                if (heartbeat_ram) {
                    uart_write_str(dbuart, "\r\nheartbeat mode set to RAM\r\n");
                }
                else {
                    uart_write_str(dbuart, "\r\nheartbeat mode set to plain (.)\r\n");
                }
            }
            else if (c == 'x') {
                heartbeat_debug(dbuart, 't');
                heartbeat_debug(dbuart, 'h');
                heartbeat_debug(dbuart, 'm');
                heartbeat_debug(dbuart, 'o');
                heartbeat_debug(dbuart, 'd');
            }
            else {
                heartbeat_debug(dbuart, c);
            }

            //if (i2sout_interruption_report) {
            //    uart_write_str(dbuart, "!");
            //    i2sout_interruption_report = false;
            //}
        }
    }

    while (true) {
        vTaskDelay(500);
        config->_userOutputs->setDigital(0, heartbeat);
        heartbeat = !heartbeat;
    }
}
