#ifndef EVENT_MGR_H
enum RmpgErr evt_mgr_init(void);
enum RmpgErr evt_mgr_on(const char *evt, void (*handle)(const char *));
enum RmpgErr evt_mgr_receive(char *buff, size_t len);
#define EVENT_MGR_H
#endif
