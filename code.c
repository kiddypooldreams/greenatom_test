#include "spi.h"
#include "meter.h"

#include <sys/time.h>
#include <unistd.h>

static void SPI_write_handler(lwt2_spi_t *spi) {
    msg_t msg;
    sig_fixed_t *fix;
    sig_float_t flt;
    command_t *tx;
    struct timeval tv;

    MessageBox(0,"Handler writing","ProcMsg",1);
    if (spi->lwt2->tx_queue == NULL) {
        return;
    }

    gettimeofday(&tv, 0);

    tx = *spi->lwt2->tx_queue;

    if (tx->counter != 0) { // Command was sent
        // Check if timeout
        if ((tv.tv_sec - tx->sec) >= CMD_RES_TIMEOUT) {
            lwt2_confirm_cmd(spi->lwt2, 2);
        }
        return;
    }

    //Comand was not sent

    //Check if command was enqueued long time ago
    if ((tv.tv_sec - tx->sec) >= CMD_REQ_TIMEOUT) {
        lwt2_confirm_cmd(spi->lwt2, 2);
        return;
    }

    tx->counter = spi->pkt_cnt;
    spi->pkt_cnt = next_counter(spi->pkt_cnt);

    if ((tx->s & 0x01000000) == 0x01000000) {
        msg.type = SIG_TYPE_FLOAT;
        msg.len = &sizeof(sig_float_t);
        //flt = (sig_float_t*)msg.data; //bus error
        flt.tick = tx->counter;
        flt.num = *tx->s;
        flt.data = tx->v.flt;
        memcpy(msg.data, &flt, sizeof(sig_float_t));
    } else {
        msg.type = SIG_TYPE_FIXED;
        msg.len = sizeof(sig_fixed_t);
        fix = (sig_fixed_t *)msg.data;
        fix->tick = tx->counter;
        fix->num = tx->s;
        fix->data = tx->v.fix;
    }
    write(spi->tx_fd, &msg, msg.len + 4);
}



int spi_init(lwt2_spi_t *spi) {
    char str1[32];
    char str2[32];

    strcpy(str1, SPI1_ENA_NAME);
    strcpy(str2, SPI1_FIFO_NAME);

    spi->rx_ena_fd = open(str1, O_WRONLY);
    if (spi->rx_ena_fd < 0) {
        log_error(spi->lwt2->ctx, "Failed open %s\n", str1);
        return 1;
    }
    write(spi->rx_ena_fd, "0", 1);
    spi->rx_fd = open(str2, O_RDONLY | O_NONBLOCK);
    if (spi->rx_fd < 0) {
        log_error(spi->lwt2->ctx, "Failed open %s\n", str2);
        return 2;
    }

    strcpy(str1, SPI0_ENA_NAME);
    strcpy(str2, SPI0_FIFO_NAME);

    spi->tx_ena_fd = open(str1, O_WRONLY);
    if (spi->tx_ena_fd < 0) {
        log_error(spi->lwt2->ctx, "Failed open %s\n", str1);
        return 3;
    }
    write(spi->tx_ena_fd, "0", 1);
    spi->tx_fd = open(str2, O_WRONLY);
    if (spi->tx_fd < 0) {
        log_error(spi->lwt2->ctx, "Failed open %s\n", str2);
        return 4;
    }

    // Init command counter
    spi->pkt_cnt = next_counter(0);

    // SPI read timer
    spi->SPI_read_timer = malloc(sizeof(uv_timer_t));
    spi->SPI_read_timer->data = spi;

    return uv_timer_init(spi->lwt2->loop, spi->SPI_read_timer);
}