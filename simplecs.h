
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#define STB_DS_IMPLEMENTATION
#include "stb_ds.h"

typedef uint64_t simplecs_entity_t;

struct Simplecs_World {
	simplecs_entity_t id;
    simplecs_entity_t * components;
} simplecs_world;

struct Simplecs_World * simplecs_init();

#define OPEN_IDS_BUFFER 128
simplecs_entity_t open_entity_ids[OPEN_IDS_BUFFER];
uint8_t num_open_entity_ids = 0;

simplecs_entity_t next_component_id = 1; // ]0,  UINT16_MAX]
simplecs_entity_t next_entity_id = UINT16_MAX + 1; // ]UINT16_MAX,  UINT64_MAX]


#define SIMPLECS_REGISTER_COMPONENT(name) struct Component_##name {\
    simplecs_entity_t id;\
    name * component;\
} component_##name;\
const Component_##name##_id = next_component_id++;
// Error if component registered twice -> user responsibility

#define SIMPLECS_HAS_COMPONENT(name, entity_id) (hmget(Component_##name, entity_id) != NULL)
#define SIMPLECS_GET_COMPONENT(name, entity_id) hmget(Component_##name, entity_id);
#define SIMPLECS_ADD_COMPONENT(world, name, entity_id) arrput(hmget(simplecs_world, entity_id), Component_##name##_id);\
malloc(hmget(Component_##name, entity_id), sizeof(Component_##name));


simplecs_entity_t simplecs_new_entity(world);