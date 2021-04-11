
#include "simplecs.h"

// Simplecs (pronounced simplex) is a very simple implementation of an entity-component-system (ECS)
// ECS is very useful in game programming.
// OOP: objects and methods, children inheriting from parents, etc.
// ECS: Entities can have any number of independent components, acted upon by systems
// Example: Videogame
//      -> main character: Physics component, PlayerControlled Component
//      -> enemies: Physics component, AIControlled Component
//      -> environment tiles: Destroyable Component
// Entities are indices (uint64_t)
// Component are structures
// Systems are functions
// The main loop iterates over systems
// There can be only one world.

struct Simplecs_World * simplecs_init() {
    simplecs_entity_t temp = SIMPLECS_NULLENTITY;
    uint8_t num_opened_entity_ids = 0;
    struct Simplecs_World * simplecs_world = (struct Simplecs_World *)calloc(sizeof(struct Simplecs_World), 1);
    simplecs_world->next_entity_id = ENTITY_ID_START; // ]UINT16_MAX,  UINT64_MAX]
    simplecs_entity_t * temp_value = NULL;
    simplecs_entity_t * temp_array = NULL;
    simplecs_world->entities_table = NULL;
    simplecs_world->num_component_combinations = 0;

    simplecs_world->component_tables = NULL;
    simplecs_world->next_system_id = 0;
    simplecs_world->component_totalnum = 0;
    simplecs_world->systems_table = (struct Simplecs_Systems_Table *)calloc(sizeof(simplecs_world->systems_table), 1);;
    simplecs_world->systems_table->components_lists = NULL;
    simplecs_world->systems_table->systems_list = NULL;
    arrput(simplecs_world->component_tables, NULL);
    return (simplecs_world);
}

simplecs_entity_t simplecs_new_entity(struct Simplecs_World * in_world) {
    simplecs_entity_t out = 0;
    simplecs_entity_t * components_list;
    while ((out == 0) && (in_world->num_opened_entity_ids > 0)) {
        out = in_world->opened_entity_ids[--in_world->num_opened_entity_ids];
        in_world->opened_entity_ids[in_world->num_opened_entity_ids] = 0;
    }
    if (out == 0) {
        out = in_world->next_entity_id++;
    }
    simplecs_entity_t temp = ENTITY_MAX_COMPONENT_NUM;
    components_list = hmget(in_world->entities_table, out);
    if (components_list != NULL) {
        arrfree(components_list);
    }
    components_list = NULL;
    arrsetcap(components_list, temp);
    hmput(in_world->entities_table, out, components_list);
    return (out);
}

simplecs_entity_t simplecs_entity_destroy(struct Simplecs_World * in_world, simplecs_entity_t in_entity) {
    simplecs_entity_t * components_list = hmget(in_world->entities_table, in_entity);
    arrfree(components_list);
    hmput(in_world->entities_table, in_entity, NULL);
    if (in_world->num_opened_entity_ids < OPEN_IDS_BUFFER) {
        in_world->opened_entity_ids[in_world->num_opened_entity_ids++] = in_entity;
    }
}

void simplecs_register_system(struct Simplecs_World * in_world, void (*in_system)(struct Simplecs_System_Input system_input), uint8_t in_run_phase, size_t num_components, ...) {
    printf("I'M IN");
    arrput(in_world->systems_table->systems_list, in_system);
    arrput(in_world->systems_table->components_num, num_components);
    simplecs_entity_t * components_list = malloc(num_components * sizeof(simplecs_entity_t));
    va_list ap;
    va_start(ap, num_components);
    for (size_t i = 0; i < num_components; i++) {
        components_list[i] = va_arg(ap, simplecs_entity_t);
    }
    arrput(in_world->systems_table->components_lists, components_list);
    in_world->next_system_id++;
}


/* PROTOTYPE SYSTEM
void Simplecs_SystemMove(struct Simplecs_System_Input system_input) {

    Position *p = SIMPLECS_COMPONENTS_LIST(entity_list, Position);

    Unit *v = SIMPLECS_COMPONENTS_LIST(entity_list, Unit);

    for (int i = 0; i < entity_num; i++) {
        p[i].x += 2;
        p[i].y += 4;
    }
}
*/