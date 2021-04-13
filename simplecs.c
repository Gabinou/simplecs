
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
    struct Simplecs_World * simplecs_world = (struct Simplecs_World *)calloc(sizeof(struct Simplecs_World), 1);
    simplecs_world->entities = NULL;
    arrsetcap(simplecs_world->entities, DEFAULT_ENTITY_CAP);
    arrput(simplecs_world->entities, 0);
    simplecs_world->entity_component_flags = NULL;
    arrsetcap(simplecs_world->entity_component_flags, DEFAULT_ENTITY_CAP);
    arrput(simplecs_world->entity_component_flags, 0);
    simplecs_world->system_typeflags = NULL;
    arrsetcap(simplecs_world->system_typeflags, DEFAULT_SYSTEM_CAP);
    simplecs_world->num_systems = 0;
    arrsetcap(simplecs_world->system_typeflags, DEFAULT_SYSTEM_CAP);
    simplecs_world->entitiesbytype = NULL;
    arrsetcap(simplecs_world->entitiesbytype, DEFAULT_SYSTEM_CAP);
    arrsetcap(simplecs_world->entitiesbytype, DEFAULT_COMPONENT_CAP);
    simplecs_world->components_bytype = NULL;
    arrsetcap(simplecs_world->entitiesbytype, DEFAULT_COMPONENT_CAP);
    simplecs_world->num_typeflags = 1;
    simplecs_world->systems = NULL;
    arrput(simplecs_world->systems, SIMPLECS_NULLTYPE);
    arrsetcap(simplecs_world->systems, DEFAULT_SYSTEM_CAP);
    simplecs_world->system_isExclusive = NULL;
    arrsetcap(simplecs_world->system_isExclusive, DEFAULT_SYSTEM_CAP);

    simplecs_world->next_entity_id = ENTITY_ID_START;
    simplecs_world->next_system_id = 0;
    simplecs_world->num_components = 0;

    return (simplecs_world);
}

simplecs_entity_t simplecs_new_entity(struct Simplecs_World * in_world) {
    simplecs_entity_t out = SIMPLECS_NULLENTITY;
    simplecs_component_t component_flag;
    while ((out == SIMPLECS_NULLENTITY) && (in_world->num_opened_entity_ids > 0)) {
        out = in_world->opened_entity_ids[--in_world->num_opened_entity_ids];
        in_world->opened_entity_ids[in_world->num_opened_entity_ids] = SIMPLECS_NULLENTITY;
    }
    if (out == SIMPLECS_NULLENTITY) {
        out = in_world->next_entity_id++;
    }
    return (out);
}

simplecs_entity_t simplecs_new_entity_wcomponents(struct Simplecs_World * in_world, simplecs_components_t component_typeflag) {

}


simplecs_entity_t simplecs_entity_destroy(struct Simplecs_World * in_world, simplecs_entity_t in_entity) {
    simplecs_component_t previous_flag = in_world->entity_component_flags[in_entity];

    for (size_t i = 0 ; i < in_world->num_systems; i++) {
        if (previous_flag == in_world->system_typeflags[i]) {
            for (size_t j = 0 ; j < in_world->num_entitiesbytype[i]; j++) {
                if (in_world->entitiesbytype[i][j] == in_entity) {
                    arrdel(in_world->entitiesbytype[i], j);
                    break;
                }
            }
        }

        in_world->entity_component_flags[in_entity] = 0;
        if (in_world->num_opened_entity_ids < OPEN_IDS_BUFFER) {
            in_world->opened_entity_ids[in_world->num_opened_entity_ids++] = in_entity;
        }
    }
}

void simplecs_register_system(struct Simplecs_World * in_world, simplecs_entity_t * entities_list, uint8_t in_run_phase, bool isexclusive, size_t component_num, simplecs_components_t component_typeflag) {
    printf("I'M IN");
    // arrput(in_world->systems_table->systems_list, in_system);
    // arrput(in_world->systems_table->components_num, num_components);
    // simplecs_entity_t * components_list = malloc(num_components * sizeof(simplecs_entity_t));
    // va_list ap;
    // va_start(ap, num_components);
    // for (size_t i = 0; i < num_components; i++) {
    //     components_list[i] = va_arg(ap, simplecs_entity_t);
    // }
    // arrput(in_world->systems_table->components_lists, components_list);
    // in_world->next_system_id++;
}

void simplecs_entity_typeflag_change(struct Simplecs_World * in_world, simplecs_entity_t in_entity, simplecs_components_t new_type) {
    simplecs_components_t previous_flag = in_world->entity_component_flags[in_entity];
    in_world->entity_component_flags[in_entity] = in_world->entity_component_flags[in_entity] | new_type;

    for (size_t i = 0; i < in_world->num_typeflags; i++) {
        if (previous_flag == in_world->typeflags[i]) { //      EXCLUSIVE
            for (size_t j = 0; j < in_world->num_entitiesbytype[i]; j++) {
                if (in_entity == in_world->entitiesbytype[i][j]) {
                    arrdel(in_world->entitiesbytype[i], j);
                    break;
                }
            }
        }
        if (in_world->entity_component_flags[in_entity] == in_world->typeflags[i]) { //      EXCLUSIVE
            arrput(in_world->entitiesbytype[i], in_entity);
        }
        // if (previous_flag & in_world->system_typeflags[i] > 0) { //   INCLUSIVE
        // if (previous_flag & in_world->system_typeflags[i] > 0) { //   INCLUSIVE
    }
}

bool simplecs_componentsbytype_migrate(struct Simplecs_World * in_world, simplecs_entity_t in_entity, simplecs_components_t old_flag, simplecs_components_t new_flag) {
    // Migrates components associated with in_entity
    // -components_bytype: previous_flag -> new_flag
    // DOES NOT CHECK in_entity's TYPE.
    size_t new_type_id = simplecs_type_id(in_world->system_typeflags, in_world->num_typeflags, new_flag);
    size_t old_type_id = simplecs_type_id(in_world->system_typeflags, in_world->num_typeflags, old_flag);

    simplecs_entity_t * new_type_entities = in_world->entitiesbytype[new_type_id];
    simplecs_entity_t * old_type_entities = in_world->entitiesbytype[old_type_id];

    size_t new_num_entities = in_world->num_entitiesbytype[new_type_id];
    size_t old_num_entities = in_world->num_entitiesbytype[old_type_id];
    size_t new_component_num = in_world->num_componentsbytype[new_type_id];
    size_t old_component_num = in_world->num_componentsbytype[old_type_id];

    struct Components_Array ** new_type_components_byentity = in_world->components_bytype[new_type_id];
    struct Components_Array ** old_type_components_byentity = in_world->components_bytype[old_type_id];

    // Deletes in_entity from old_type_entities
    size_t found_old = 0;
    for (size_t i = 0; i < old_num_entities; i++) {
        if (old_type_entities[i] == in_entity) {
            arrdel(old_type_entities, i);
            in_world->num_entitiesbytype[old_type_id]--;
            found_old = i;
        }
    }
    size_t found_new = 0;
    for (size_t i = 0; i < new_num_entities; i++) {
        if (new_type_entities[i] == in_entity) {
            found_new = i;
        }
    }
    if (!found_new) {
        arrput(new_type_entities, in_entity);
        in_world->entitiesbytype[new_type_id]++;
    } else {
        printf("entity found in components_bytype for new_flag");
    }

    if (found_old && !found_new) {
        arrput(new_type_components_byentity, old_type_components_byentity[found_old]);
        arrdel(old_type_components_byentity, found_old);
    } else {
        printf("entity found in components_bytype for new_flag");
    }

}

size_t simplecs_type_id(simplecs_components_t * in_typelist, size_t len, simplecs_components_t in_flag) {
    size_t found = SIMPLECS_NULLTYPE;
    for (size_t i = 0; i < len; i++) {
        if (in_typelist[i] == in_flag) {
            found = i;
            break;
        }
    }
    return (found);
}


size_t simplecs_issubtype(simplecs_components_t * in_typelist, size_t len, simplecs_components_t in_flag) {
    // returns position of subtype from in_typelist
    size_t found = 0;
    for (size_t i = 0; i < len; i++) {
        if ((in_typelist[i] & in_flag) == in_flag) {
            found = i;
            break;
        }
    }
    return (found);
}
