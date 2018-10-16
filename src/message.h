typedef struct {
    int source;
    int type;
    int start;
    int stop;
} msg_t;

void initMsg(msg_t *dst, int source, int type, int start, int stop);

void sendMsg(int to, msg_t *src);
void recvMsg(int from, msg_t *dst);

void initMailboxes(int numMailboxes);
void cleanupMailboxes();