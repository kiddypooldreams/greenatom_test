typedef struct {
    int tx_fd, rx_fd;
    int pkt_cnt;
    void *lwt2;
} lwt2_spi_t;

#define CMD_RES_TIMEOUT 5
#define CMD_REQ_TIMEOUT 10