
#include "simplecs.h"

// Simplecs (pronounced simplex) is a very simple implementation of an entity-component-system (ECS)
// ECS is very useful in game programming. 
// OOP: objects and methods, children inheriting from parents, etc.
// ECS: Entities can have any number of independent components, acted upon by systems
// Example: Videogame 
//      -> main character: Physics component, PlayerControlled Component
//      -> enemies: Physics component, AIControlled Component
//      -> Environment tiles: Destroyable Component
// 
// Entities are indices (uint64_t)
// Component are structures
// Systems are functions
// The main loop iterates over systems

uint8_t num_opened_entity_ids = 0;
simplecs_entity_t next_component_id = 1; // ]0,  UINT16_MAX]
simplecs_entity_t next_entity_id = UINT16_MAX + 1; // ]UINT16_MAX,  UINT64_MAX]

struct Simplecs_World * simplecs_init() {
    simplecs_entity_t temp = 1;
    simplecs_entity_t * temp_value = NULL;
    simplecs_world = NULL;
    hmdefault(simplecs_world, temp_value);
    simplecs_entity_t * temp_array = NULL;
    // temp_array = (simplecs_entity_t *) malloc(100);
    arrput(temp_array, 1);
    printf("HERE\n");
    hmput(simplecs_world, temp, temp_array);
    printf("HERE\n");
    // arrput(component_tables, NULL);
	return(&simplecs_world);
}

simplecs_entity_t simplecs_new_entity(struct Simplecs_World * in_world) {
	simplecs_entity_t out = 0;
	while ((out == 0) && (num_opened_entity_ids > 0)) {
		out = opened_entity_ids[--num_opened_entity_ids];
		opened_entity_ids[num_opened_entity_ids] = 0;
	}
	if (out == 0) {
		out = next_entity_id++;
	} 
    return(out);
}

simplecs_entity_t simplecs_entity_destroy(struct Simplecs_World * in_world, simplecs_entity_t in_entity) {
    for (size_t i = 0; i < arrlen(hmget(in_world, in_entity)); i++) {
        free(component_tables[hmget(in_world, in_entity)[i]]);
    }
    hmput(in_world, in_entity, NULL);
    if (num_opened_entity_ids < OPEN_IDS_BUFFER) {
        opened_entity_ids[num_opened_entity_ids++] = in_entity;
    }
}

