#ifndef TEST_EVENT_GROUPS_H
#define TEST_EVENT_GROUPS_H
#include <stdint.h>
typedef uint32_t EventBits_t;
typedef struct { EventBits_t bits; } StaticEventGroup_t;
typedef StaticEventGroup_t *EventGroupHandle_t;
EventGroupHandle_t xEventGroupCreateStatic(StaticEventGroup_t *storage);
EventBits_t xEventGroupSetBits(EventGroupHandle_t group, EventBits_t bits);
EventBits_t xEventGroupClearBits(EventGroupHandle_t group, EventBits_t bits);
EventBits_t xEventGroupGetBits(EventGroupHandle_t group);
#endif
