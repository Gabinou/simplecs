
#include "tnecs.h"

/********************** 0.1 MICROSECOND RESOLUTION CLOCK **********************/
uint64_t tnecs_get_ns() {
    static uint64_t is_init = 0;
#if defined(__APPLE__)
    static mach_timebase_info_data_t info;
    if (0 == is_init) {
        mach_timebase_info(&info);
        is_init = 1;
    }
    uint64_t now;
    now = mach_absolute_time();
    now *= info.numer;
    now /= info.denom;
    return now;
#elif defined(__linux)
    static struct timespec linux_rate;
    if (0 == is_init) {
        clock_getres(CLOCKID, &linux_rate);
        is_init = 1;
    }
    uint64_t now;
    struct timespec spec;
    clock_gettime(CLOCKID, &spec);
    now = spec.tv_sec * 1.0e9 + spec.tv_nsec;
    return now;
#elif defined(_WIN32)
    static LARGE_INTEGER win_frequency;
    if (0 == is_init) {
        QueryPerformanceFrequency(&win_frequency);
        is_init = 1;
    }
    LARGE_INTEGER now;
    QueryPerformanceCounter(&now);
    return (uint64_t)((1e9 * now.QuadPart) / win_frequency.QuadPart);
#endif
}
#ifdef MICROSECOND_CLOCK
double tnecs_get_us() {
    return (tnecs_get_ns() / 1e3);
}
#else
#  define FAILSAFE_CLOCK
#  define tnecs_get_us() (((double)clock())/CLOCKS_PER_SEC*1e6) // [us]
#  define tnecs_get_ns() (((double)clock())/CLOCKS_PER_SEC*1e9) // [ns]
#endif

/**************************** WORLD FUNCTIONS ********************************/
struct tnecs_World * tnecs_world_genesis() {
    TNECS_DEBUG_PRINTF("tnecs_world_genesis\n");

    struct tnecs_World * tnecs_world = (struct tnecs_World *)calloc(sizeof(struct tnecs_World), 1);
    if (tnecs_world) {
        bool success = 1;
        success &= tnecs_world_breath_entities(tnecs_world);
        success &= tnecs_world_breath_typeflags(tnecs_world);
        success &= tnecs_world_breath_systems(tnecs_world);
        success &= tnecs_world_breath_components(tnecs_world);
        TNECS_DEBUG_ASSERT(success);
    }
    return (tnecs_world);
}

void tnecs_world_destroy(struct tnecs_World * in_world) {
    TNECS_DEBUG_PRINTF("tnecs_world_destroy\n");
    for (size_t i = 0; i < in_world->len_phases; i++) {
        free(in_world->systems_byphase[i]);
        free(in_world->systems_idbyphase[i]);
    }
    for (size_t i = 0; i < in_world->len_typeflags; i++) {
        free(in_world->entities_bytype[i]);
        free(in_world->components_idbytype[i]);
        free(in_world->components_flagbytype[i]);
        free(in_world->components_orderbytype[i]);
        free(in_world->supertype_id_bytype[i]);
        for (size_t j = 0; j < in_world->num_components_bytype[i]; j++) {
            free(in_world->components_bytype[i][j].components);
        }
        free(in_world->components_bytype[i]);
    }
    for (size_t i = 0; i < in_world->num_components; i++) {
        free(in_world->component_names[i]);
    }
    for (size_t i = 0; i < in_world->num_systems; i++) {
        free(in_world->system_names[i]);
    }
    free(in_world->components_bytype);
    free(in_world->components_idbytype);
    free(in_world->components_flagbytype);
    free(in_world->components_orderbytype);
    free(in_world->entities_bytype);
    free(in_world->entity_orders);
    free(in_world->entities);
    free(in_world->entities_open);
    free(in_world->entity_typeflags);
    free(in_world->len_entities_bytype);
    free(in_world->len_systems_byphase);
    free(in_world->num_entities_bytype);
    free(in_world->num_systems_byphase);
    free(in_world->num_supertype_ids);
    free(in_world->num_components_bytype);
    free(in_world->phases);
    free(in_world->supertype_id_bytype);
    free(in_world->systems_byphase);
    free(in_world->system_orders);
    free(in_world->system_exclusive);
    free(in_world->systems_torun);
    free(in_world->systems_idbyphase);
    free(in_world->system_typeflags);
    free(in_world->system_phases);
    free(in_world->system_hashes);
    free(in_world->system_names);
    free(in_world->typeflags);
    free(in_world);
}

void tnecs_world_step(struct tnecs_World * in_world, tnecs_time_ns_t in_deltat) {
    TNECS_DEBUG_PRINTF("tnecs_world_step\n");

    struct tnecs_System_Input current_input;
    current_input.world = in_world;
    size_t system_id, current_phase, system_typeflag_id;
    in_world->num_systems_torun = 0;
    if (!in_deltat) {
        current_input.deltat = tnecs_get_ns() - in_world->previous_time;
    } else {
        current_input.deltat = in_deltat;
    }
    for (size_t phase_id = 0; phase_id < in_world->num_phases; phase_id++) {
        current_phase = in_world->phases[phase_id];
        if (phase_id == current_phase) {
            for (size_t sorder = 0; sorder < in_world->num_systems_byphase[phase_id]; sorder++) {
                system_id = in_world->systems_idbyphase[phase_id][sorder];
                system_typeflag_id = tnecs_typeflagid(in_world, in_world->system_typeflags[system_id]);
                current_input.entity_typeflag_id = system_typeflag_id;
                current_input.num_entities = in_world->num_entities_bytype[current_input.entity_typeflag_id];
                if (in_world->num_systems_torun >= (in_world->len_systems_torun-1)) {
                    size_t old_len = in_world->len_systems_torun;
                    in_world->len_systems_torun *= TNECS_ARRAY_GROWTH_FACTOR;
                    in_world->systems_torun = tnecs_realloc(in_world->systems_torun, old_len, in_world->len_systems_torun, sizeof(*in_world->systems_torun));
                }
                in_world->systems_torun[in_world->num_systems_torun++] = in_world->systems_byphase[phase_id][sorder];
                in_world->systems_byphase[phase_id][sorder](&current_input);
                if (!in_world->system_exclusive[system_id]) {
                    for (size_t tsub = 0; tsub < in_world->num_supertype_ids[system_typeflag_id]; tsub++) {
                        current_input.entity_typeflag_id = in_world->supertype_id_bytype[system_typeflag_id][tsub];
                        current_input.num_entities = in_world->num_entities_bytype[current_input.entity_typeflag_id];
                        in_world->systems_torun[in_world->num_systems_torun++] = in_world->systems_byphase[phase_id][sorder];
                        in_world->systems_byphase[phase_id][sorder](&current_input);
                    }
                }
            }
        }
    }
    in_world->previous_time = tnecs_get_ns();
}

bool tnecs_world_breath_entities(struct tnecs_World * in_world) {
    TNECS_DEBUG_PRINTF("tnecs_world_breath_entities\n");

    bool success = 1;
    in_world->entity_next = TNECS_NULLSHIFT;
    in_world->num_entities_open = 0;
    in_world->len_entities = TNECS_INITIAL_ENTITY_LEN;
    in_world->len_entities_open = TNECS_INITIAL_ENTITY_LEN;
    success &= ((in_world->entities = calloc(TNECS_INITIAL_ENTITY_LEN, sizeof(*in_world->entities))) != NULL);
    success &= ((in_world->entities_open = calloc(TNECS_INITIAL_ENTITY_LEN, sizeof(*in_world->entities_open))) != NULL);
    success &= ((in_world->entity_typeflags = calloc(TNECS_INITIAL_ENTITY_LEN, sizeof(*in_world->entity_typeflags))) != NULL);
    success &= ((in_world->entity_orders = calloc(TNECS_INITIAL_ENTITY_LEN, sizeof(*in_world->entity_orders))) != NULL);
    success &= ((in_world->entities_bytype = calloc(TNECS_INITIAL_SYSTEM_LEN, sizeof(*in_world->entities_bytype))) != NULL);
    success &= ((in_world->len_entities_bytype = calloc(TNECS_INITIAL_SYSTEM_LEN, sizeof(*in_world->len_entities_bytype))) != NULL);
    success &= ((in_world->num_entities_bytype = calloc(TNECS_INITIAL_SYSTEM_LEN, sizeof(*in_world->num_entities_bytype))) != NULL);
    for (size_t i = 0; i < TNECS_INITIAL_SYSTEM_LEN; i++) {
        success &= ((in_world->entities_bytype[i] = calloc(TNECS_INITIAL_ENTITY_LEN, sizeof(**in_world->entities_bytype))) != NULL);
        in_world->num_entities_bytype[i] = 0;
        in_world->len_entities_bytype[i] = TNECS_INITIAL_ENTITY_LEN;
    }
    return (success);
}

bool tnecs_world_breath_components(struct tnecs_World * in_world) {
    TNECS_DEBUG_PRINTF("tnecs_world_breath_components\n");

    bool success = 1;
    in_world->component_hashes[TNECS_NULL] = TNECS_NULL;
    success &= ((in_world->components_bytype = calloc(TNECS_INITIAL_SYSTEM_LEN, sizeof(*in_world->components_bytype))) != NULL);
    success &= ((in_world->num_components_bytype = calloc(TNECS_INITIAL_SYSTEM_LEN, sizeof(*in_world->num_components_bytype))) != NULL);
    success &= ((in_world->components_idbytype = calloc(TNECS_INITIAL_SYSTEM_LEN, sizeof(*in_world->components_idbytype))) != NULL);
    success &= ((in_world->components_flagbytype = calloc(TNECS_INITIAL_SYSTEM_LEN, sizeof(*in_world->components_flagbytype))) != NULL);
    success &= ((in_world->components_orderbytype = calloc(TNECS_INITIAL_SYSTEM_LEN, sizeof(*in_world->components_orderbytype))) != NULL);
    for (size_t i = 0; i < TNECS_INITIAL_SYSTEM_LEN; i++) {
        in_world->num_components_bytype[i] = 0;
    }
    in_world->num_components = TNECS_NULLSHIFT;
    success &= ((in_world->component_names[0] = malloc(5)) != NULL);
    strncpy(in_world->component_names[0], "NULL", 5);
    return (success);
}

bool tnecs_world_breath_systems(struct tnecs_World * in_world) {
    TNECS_DEBUG_PRINTF("tnecs_world_breath_systems\n");

    bool success = 1;
    in_world->len_systems = TNECS_INITIAL_SYSTEM_LEN;
    in_world->len_systems_torun = TNECS_INITIAL_SYSTEM_LEN;
    in_world->num_systems = TNECS_NULLSHIFT;
    // success &= ((in_world->systems_torun = calloc(in_world->len_systems, sizeof(*in_world->systems_torun))) != NULL);
    in_world->systems_torun = calloc(in_world->len_systems_torun, sizeof(*in_world->systems_torun));
    assert(in_world->systems_torun);
    success &= ((in_world->system_typeflags = calloc(in_world->len_systems, sizeof(*in_world->system_typeflags))) != NULL);
    success &= ((in_world->system_hashes = calloc(in_world->len_systems, sizeof(*in_world->system_hashes))) != NULL);
    success &= ((in_world->system_phases = calloc(in_world->len_systems, sizeof(*in_world->system_phases))) != NULL);
    success &= ((in_world->system_orders = calloc(in_world->len_systems, sizeof(*in_world->system_orders))) != NULL);
    success &= ((in_world->system_exclusive = calloc(in_world->len_systems, sizeof(*in_world->system_exclusive))) != NULL);
    success &= ((in_world->system_names = calloc(in_world->len_systems, sizeof(*in_world->system_names))) != NULL);
    in_world->len_phases = TNECS_INITIAL_PHASE_LEN;
    in_world->num_phases = TNECS_NULLSHIFT;
    success &= ((in_world->phases = calloc(in_world->len_phases, sizeof(*in_world->phases))) != NULL);
    success &= ((in_world->systems_idbyphase = calloc(in_world->len_phases, sizeof(*in_world->systems_idbyphase))) != NULL);
    success &= ((in_world->len_systems_byphase = calloc(in_world->len_phases, sizeof(*in_world->len_systems_byphase))) != NULL);
    success &= ((in_world->num_systems_byphase = calloc(in_world->len_phases, sizeof(*in_world->num_systems_byphase))) != NULL);
    success &= ((in_world->systems_byphase = calloc(in_world->len_phases, sizeof(*in_world->systems_byphase))) != NULL);
    for (size_t i = 0; i < in_world->len_phases; i++) {
        success &= ((in_world->systems_byphase[i] = calloc(in_world->len_phases, sizeof(**in_world->systems_byphase))) != NULL);
        success &= ((in_world->systems_idbyphase[i] = calloc(in_world->len_phases, sizeof(**in_world->systems_idbyphase))) != NULL);
        in_world->num_systems_byphase[i] = 0;
        in_world->len_systems_byphase[i] = in_world->len_phases;
    }
    success &= (in_world->system_names[0] = malloc(5)) != NULL;
    strncpy(in_world->system_names[0], "NULL", 5);
    return (success);
}

bool tnecs_world_breath_typeflags(struct tnecs_World * in_world) {
    TNECS_DEBUG_PRINTF("tnecs_world_breath_typeflags\n");

    bool success = 1;
    success &= ((in_world->typeflags = calloc(TNECS_INITIAL_SYSTEM_LEN, sizeof(*in_world->typeflags))) != NULL);
    success &= ((in_world->supertype_id_bytype = calloc(TNECS_INITIAL_SYSTEM_LEN, sizeof(*in_world->supertype_id_bytype))) != NULL);
    success &= ((in_world->num_supertype_ids = calloc(TNECS_INITIAL_SYSTEM_LEN, sizeof(*in_world->num_supertype_ids))) != NULL);
    for (size_t i = 0; i < TNECS_INITIAL_SYSTEM_LEN; i++) {
        success &= ((in_world->supertype_id_bytype[i] = calloc(TNECS_COMPONENT_CAP, sizeof(**in_world->supertype_id_bytype))) != NULL);
    }
    in_world->num_typeflags = TNECS_NULLSHIFT;
    in_world->len_typeflags = TNECS_INITIAL_SYSTEM_LEN;
    return (success);
}

/***************************** REGISTRATION **********************************/
size_t tnecs_register_system(struct tnecs_World * in_world, const char * in_name, void (* in_system)(struct tnecs_System_Input *), tnecs_phase_t in_phase, bool in_isExclusive, size_t num_components, tnecs_component_t components_typeflag) {
    TNECS_DEBUG_PRINTF("tnecs_register_system\n");

    size_t system_id = in_world->num_systems++;
    in_world->system_names[system_id] = malloc(strlen(in_name) + 1);
    strncpy(in_world->system_names[system_id], in_name, strlen(in_name) + 1);
    tnecs_hash_t in_hash = tnecs_hash_djb2(in_name);
    if (in_world->num_systems >= in_world->len_systems) {
        tnecs_growArray_system(in_world);
    }
    if (!in_world->phases[in_phase]) {
        tnecs_register_phase(in_world, in_phase);
    }
    in_world->system_exclusive[system_id] = in_isExclusive;
    in_world->system_phases[system_id] = in_phase;
    in_world->system_hashes[system_id] = in_hash;
    in_world->system_typeflags[system_id] = components_typeflag;
    size_t system_order = in_world->num_systems_byphase[in_phase]++;
    if (in_world->num_systems_byphase[in_phase] >= in_world->len_systems_byphase[in_phase]) {
        size_t old_len = in_world->len_systems_byphase[in_phase];
        in_world->len_systems_byphase[in_phase] *= TNECS_ARRAY_GROWTH_FACTOR;
        in_world->systems_byphase[in_phase] = tnecs_realloc(in_world->systems_byphase[in_phase], old_len, in_world->len_systems_byphase[in_phase], sizeof(**in_world->systems_byphase));
        in_world->systems_idbyphase[in_phase] = tnecs_realloc(in_world->systems_idbyphase[in_phase], old_len, in_world->len_systems_byphase[in_phase], sizeof(**in_world->systems_idbyphase));
    }
    in_world->system_orders[system_id] = system_order;
    in_world->systems_byphase[in_phase][system_order] = in_system;
    in_world->systems_idbyphase[in_phase][system_order] = system_id;
    tnecs_register_typeflag(in_world, num_components, components_typeflag);
    return (system_id);
}

tnecs_component_t tnecs_register_component(struct tnecs_World * in_world, const char * in_name, size_t in_bytesize) {
    TNECS_DEBUG_PRINTF("tnecs_register_component\n");

    tnecs_component_t new_component_id = 0;
    if (in_world->num_components < TNECS_COMPONENT_CAP) {
        new_component_id = in_world->num_components++;
        in_world->component_hashes[new_component_id] = tnecs_hash_djb2(in_name);
        tnecs_component_t new_component_flag = TNECS_COMPONENT_ID2TYPE(new_component_id);
        TNECS_DEBUG_ASSERT(in_bytesize > 0);
        in_world->component_bytesizes[new_component_id] = in_bytesize;
        in_world->component_names[new_component_id] = malloc(strlen(in_name) + 1);
        strncpy(in_world->component_names[new_component_id], in_name, strlen(in_name) + 1);
        size_t typeflag_id = tnecs_register_typeflag(in_world, 1, new_component_flag);
    }
    return (new_component_id);
}

size_t tnecs_register_typeflag(struct tnecs_World * in_world, size_t num_components, tnecs_component_t typeflag_new) {
    TNECS_DEBUG_PRINTF("tnecs_new_typeflag\n");

    size_t typeflag_id = 0;
    for (size_t i = 0 ; i < in_world->num_typeflags; i++) {
        if (typeflag_new == in_world->typeflags[i]) {
            typeflag_id = i;
            break;
        }
    }
    if (!typeflag_id) {
        // 1- Add new components_bytype at [typeflag_id]
        if ((in_world->num_typeflags + 1) >= in_world->len_typeflags) {
            tnecs_growArray_typeflag(in_world);
        }
        in_world->typeflags[in_world->num_typeflags++] = typeflag_new;
        typeflag_id = tnecs_typeflagid(in_world, typeflag_new);
        TNECS_DEBUG_ASSERT(typeflag_id == (in_world->num_typeflags - 1));
        in_world->num_components_bytype[typeflag_id] = num_components;
        // 2- Add arrays to components_bytype[typeflag_id] for each component
        tnecs_component_array_new(in_world, num_components, typeflag_new);
        // 3- Add all components to components_idbytype and components_flagbytype
        tnecs_component_t component_id_toadd, component_type_toadd;
        tnecs_component_t typeflag_reduced = typeflag_new, typeflag_added = 0;
        in_world->components_idbytype[typeflag_id] =  calloc(num_components, sizeof(**in_world->components_idbytype));
        in_world->components_flagbytype[typeflag_id] =  calloc(num_components, sizeof(**in_world->components_flagbytype));
        in_world->components_orderbytype[typeflag_id] =  calloc(TNECS_COMPONENT_CAP, sizeof(**in_world->components_orderbytype));
        size_t i = 0;
        while (typeflag_reduced) {
            typeflag_reduced &= (typeflag_reduced - 1);
            component_type_toadd = (typeflag_reduced + typeflag_added) ^ typeflag_new;
            component_id_toadd = TNECS_COMPONENT_TYPE2ID(component_type_toadd);
            in_world->components_idbytype[typeflag_id][i] = component_id_toadd;
            in_world->components_flagbytype[typeflag_id][i] = component_type_toadd;
            in_world->components_orderbytype[typeflag_id][component_id_toadd] = i;
            typeflag_added += component_type_toadd;
            i++;
        }
        // 4- Check supertypes.
        for (size_t i = 1 ; i < in_world->num_typeflags; i++) {
            in_world->num_supertype_ids[i] = 0;
            for (size_t j = 1 ; j < (in_world->num_typeflags); j++) {
                if (i != j) {
                    if (TNECS_TYPEFLAG_IS_SUPERTYPE(in_world->typeflags[i], in_world->typeflags[j])) {
                        in_world->supertype_id_bytype[i][in_world->num_supertype_ids[i]++] = j; // j is a supertype of i
                    }
                }
            }
        }
    }
    return (typeflag_id);
}

size_t tnecs_register_phase(struct tnecs_World * in_world, tnecs_phase_t in_phase) {
    TNECS_DEBUG_PRINTF("tnecs_register_phase\n");
    while (in_phase >= in_world->len_phases) {
        tnecs_growArray_phase(in_world);
    }
    in_world->phases[in_phase] = in_phase;
    in_world->num_phases = (in_phase >= in_world->num_phases) ? (in_phase + 1) : in_world->num_phases;
    return (in_phase);
}

/***************************** ENTITY MANIPULATION ***************************/
tnecs_entity_t tnecs_entity_create(struct tnecs_World * in_world) {
    TNECS_DEBUG_PRINTF("tnecs_entity_create\n");

    tnecs_entity_t out = TNECS_NULL;
    tnecs_component_t component_flag;
    while ((out == TNECS_NULL) && (in_world->num_entities_open > 0)) {
        out = in_world->entities_open[--in_world->num_entities_open];
        in_world->entities_open[in_world->num_entities_open] = TNECS_NULL;
    }
    if (out == TNECS_NULL) {
        do {
            if (in_world->entity_next >= in_world->len_entities) {
                tnecs_growArray_entity(in_world);
            }
            out = in_world->entity_next;
        } while (in_world->entities[in_world->entity_next++] != TNECS_NULL);
    }
    TNECS_DEBUG_ASSERT(out != TNECS_NULL);
    in_world->entities[out] = out;
    tnecs_entitiesbytype_add(in_world, out, TNECS_NULL);
    TNECS_DEBUG_ASSERT(in_world->entities[out] == out);
    TNECS_DEBUG_ASSERT(in_world->entities_bytype[TNECS_NULL][in_world->entity_orders[out]] == out);
    return (out);
}

tnecs_entity_t tnecs_entity_create_windex(struct tnecs_World * in_world, tnecs_entity_t in_entity) {
    TNECS_DEBUG_PRINTF("tnecs_entity_create_windex\n");

    tnecs_entity_t out = 0;
    while (in_entity >= in_world->len_entities) {
        tnecs_growArray_entity(in_world);
    }
    if ((!in_world->entities[in_entity]) & (in_entity > 0)) {
        out = in_world->entities[in_entity] = in_entity;
        tnecs_entitiesbytype_add(in_world, out, TNECS_NULL);
    }
    return (out);
}

tnecs_entity_t tnecs_entities_create(struct tnecs_World * in_world, size_t num) {
    TNECS_DEBUG_PRINTF("tnecs_entities_create\n");

    size_t created = 0;
    while (created < num) {
        created += (tnecs_entity_create(in_world) > 0);
    }
    return (created);
}

tnecs_entity_t tnecs_entities_create_windices(struct tnecs_World * in_world, size_t num, tnecs_entity_t * in_entities) {
    TNECS_DEBUG_PRINTF("tnecs_entities_create_windices\n");

    size_t created = 0;
    for (size_t i = 0; i < num; i++) {
        created += (tnecs_entity_create_windex(in_world, in_entities[i]) > 0);
    }
    return (created);
}

tnecs_entity_t tnecs_entity_create_wcomponents(struct tnecs_World * in_world, size_t argnum, ...) {
    TNECS_DEBUG_PRINTF("tnecs_entity_create_wcomponents \n");

    va_list ap;
    va_start(ap, argnum);
    tnecs_component_t typeflag = 0;
    tnecs_hash_t current_hash;
    for (size_t i = 0; i < argnum; i++) {
        current_hash = va_arg(ap, tnecs_hash_t);
        typeflag += tnecs_component_hash2type(in_world, current_hash);
    }
    va_end(ap);
    tnecs_entity_t new_entity = tnecs_entity_create(in_world);
    tnecs_entity_add_components(in_world, new_entity, argnum, typeflag, true);
    size_t typeflag_id = TNECS_TYPEFLAGID(in_world, typeflag);
    TNECS_DEBUG_ASSERT(in_world->entities_bytype[typeflag_id][in_world->entity_orders[new_entity]] == new_entity);
    TNECS_DEBUG_ASSERT((in_world->entities[new_entity] = new_entity));
    return (new_entity);
}

tnecs_entity_t tnecs_entity_destroy(struct tnecs_World * in_world, tnecs_entity_t in_entity) {
    TNECS_DEBUG_PRINTF("tnecs_entity_destroy \n");

    TNECS_DEBUG_ASSERT(in_world->entities[in_entity]);
    TNECS_DEBUG_ASSERT(in_entity > 0);
    tnecs_component_t entity_typeflag = in_world->entity_typeflags[in_entity];
    TNECS_DEBUG_ASSERT(entity_typeflag >= 0);
    size_t entity_typeflag_id = TNECS_TYPEFLAGID(in_world, entity_typeflag);
    size_t entity_order = in_world->entity_orders[in_entity];
    TNECS_DEBUG_ASSERT(in_world->num_entities_bytype[entity_typeflag_id] > 0);
    tnecs_component_del(in_world, in_entity, entity_typeflag);
    TNECS_DEBUG_ASSERT(in_world->len_entities_bytype[entity_typeflag_id] >= entity_order);
    TNECS_DEBUG_ASSERT(in_world->num_entities_bytype[entity_typeflag_id] > 0);
    tnecs_entitiesbytype_del(in_world, in_entity, entity_typeflag);
    in_world->entities[in_entity] = TNECS_NULL;
    in_world->entity_typeflags[in_entity] = TNECS_NULL;
    in_world->entity_orders[in_entity] = TNECS_NULL;
    if ((in_world->num_entities_open + 1) >= in_world->len_entities_open) {
        size_t old_len = in_world->len_entities_open;
        in_world->len_entities_open *= TNECS_ARRAY_GROWTH_FACTOR;
        in_world->entities_open = tnecs_realloc(in_world->entities_open, old_len, in_world->len_entities_open, sizeof(*in_world->entities_open));
    }
    in_world->entities_open[in_world->num_entities_open++] = in_entity;
    TNECS_DEBUG_ASSERT(in_world->entities[in_entity] == TNECS_NULL);
    TNECS_DEBUG_ASSERT(in_world->entity_typeflags[in_entity] == TNECS_NULL);
    TNECS_DEBUG_ASSERT(in_world->entity_orders[in_entity] == TNECS_NULL);
    TNECS_DEBUG_ASSERT(in_world->entity_orders[entity_order] != in_entity);
    return (in_world->entities[in_entity] == 0);
}

/*****************************************************************************/
/***************************** TNECS INTERNALS *******************************/
/*****************************************************************************/
tnecs_entity_t tnecs_entity_add_components(struct tnecs_World * in_world, tnecs_entity_t in_entity, size_t num_components_toadd, tnecs_component_t typeflag_toadd, bool isNew) {
    TNECS_DEBUG_PRINTF("tnecs_entity_add_components\n");

    tnecs_component_t typeflag_old = in_world->entity_typeflags[in_entity];
    TNECS_DEBUG_ASSERT((typeflag_toadd != typeflag_old));
    tnecs_component_t typeflag_new = typeflag_toadd + typeflag_old;
    if (isNew) {
        tnecs_register_typeflag(in_world, setBits_KnR_uint64_t(typeflag_new), typeflag_new);
    }
    size_t typeflag_new_id = tnecs_typeflagid(in_world, typeflag_new);
    tnecs_component_migrate(in_world, in_entity, typeflag_old, typeflag_new);
    tnecs_entitiesbytype_migrate(in_world, in_entity, typeflag_old, typeflag_new);
    TNECS_DEBUG_ASSERT(in_world->entity_typeflags[in_entity] == typeflag_new);
    TNECS_DEBUG_ASSERT(in_world->entities_bytype[typeflag_new_id][in_world->num_entities_bytype[typeflag_new_id] - 1] == in_entity);
    TNECS_DEBUG_ASSERT(in_world->entity_orders[in_entity] == in_world->num_entities_bytype[typeflag_new_id] - 1);
    return (in_world->entities[in_entity]);
}

void * tnecs_entity_get_component(struct tnecs_World * in_world, tnecs_entity_t in_entity_id, tnecs_component_t in_component_id) {
    TNECS_DEBUG_PRINTF("tnecs_entity_get_component\n");

    tnecs_component_t component_flag = TNECS_COMPONENT_ID2TYPE(in_component_id);
    tnecs_component_t entity_typeflag = TNECS_ENTITY_TYPEFLAG(in_world, in_entity_id);
    void * out = NULL;
    if ((component_flag & entity_typeflag) > 0) {
        size_t typeflag_id = tnecs_typeflagid(in_world, entity_typeflag);
        size_t component_order = tnecs_component_order_bytype(in_world, in_component_id, entity_typeflag);
        TNECS_DEBUG_ASSERT(component_order <= in_world->num_components_bytype[typeflag_id]);
        size_t entity_order = in_world->entity_orders[in_entity_id];
        size_t bytesize = in_world->component_bytesizes[in_component_id];
        struct tnecs_Components_Array * comp_array = &in_world->components_bytype[typeflag_id][component_order];
        tnecs_byte_t * temp_component_bytesptr = (tnecs_byte_t *)(comp_array->components);
        out = (temp_component_bytesptr + (bytesize * entity_order));
    }
    return (out);
}

size_t tnecs_entitiesbytype_add(struct tnecs_World * in_world, tnecs_entity_t in_entity, tnecs_component_t typeflag_new) {
    TNECS_DEBUG_PRINTF("tnecs_entitiesbytype_add\n");

    size_t typeflag_id_new = tnecs_typeflagid(in_world, typeflag_new);
    if ((in_world->num_entities_bytype[typeflag_id_new] + 1) >= in_world->len_entities_bytype[typeflag_id_new]) {
        tnecs_growArray_bytype(in_world, typeflag_id_new);
    }
    in_world->entities_bytype[typeflag_id_new][in_world->num_entities_bytype[typeflag_id_new]] = in_entity;
    in_world->entity_typeflags[in_entity] = typeflag_new;
    in_world->entity_orders[in_entity] = in_world->num_entities_bytype[typeflag_id_new]++;
    return (in_world->entity_orders[in_entity]);
}

size_t tnecs_entitiesbytype_del(struct tnecs_World * in_world, tnecs_entity_t in_entity, tnecs_component_t typeflag_old) {
    TNECS_DEBUG_PRINTF("tnecs_entitiesbytype_del\n");

    TNECS_DEBUG_ASSERT(in_entity);
    TNECS_DEBUG_ASSERT(in_world->entities[in_entity] == in_entity);
    TNECS_DEBUG_ASSERT(in_entity < in_world->len_entities);
    size_t typeflag_old_id = tnecs_typeflagid(in_world, typeflag_old);
    TNECS_DEBUG_ASSERT(in_world->num_entities_bytype[typeflag_old_id] > 0);
    size_t entity_order_old = in_world->entity_orders[in_entity];
    TNECS_DEBUG_ASSERT(in_world->entities_bytype[typeflag_old_id][entity_order_old] == in_entity);
    TNECS_DEBUG_ASSERT(entity_order_old < in_world->len_entities_bytype[typeflag_old_id]);
    TNECS_DEBUG_ASSERT(in_world->entities_bytype[typeflag_old_id][entity_order_old] == in_entity);
    tnecs_entity_t top_entity = in_world->entities_bytype[typeflag_old_id][--in_world->num_entities_bytype[typeflag_old_id]];
    size_t top_entity_order = in_world->entity_orders[top_entity];
    TNECS_DEBUG_ASSERT(in_world->entities_bytype[typeflag_old_id][top_entity_order] == top_entity);
    if (top_entity != in_entity) {
        in_world->entities_bytype[typeflag_old_id][top_entity_order] = TNECS_NULL;
        in_world->entity_orders[top_entity] = entity_order_old;
        in_world->entities_bytype[typeflag_old_id][entity_order_old] = top_entity;
        TNECS_DEBUG_ASSERT(in_world->entities_bytype[typeflag_old_id][in_world->entity_orders[top_entity]] == top_entity);
    }
    in_world->entity_orders[in_entity] = TNECS_NULL;
    in_world->entity_typeflags[in_entity] = TNECS_NULL;
    return ((in_world->entity_orders[in_entity] == TNECS_NULL) && (in_world->entity_typeflags[in_entity]));
}

size_t tnecs_entitiesbytype_migrate(struct tnecs_World * in_world, tnecs_entity_t in_entity, tnecs_component_t typeflag_old, tnecs_component_t typeflag_new) {
    TNECS_DEBUG_PRINTF("tnecs_entitiesbytype_migrate\n");

    tnecs_entitiesbytype_del(in_world, in_entity, typeflag_old);
    TNECS_DEBUG_ASSERT(in_world->entity_typeflags[in_entity] == TNECS_NULL);
    TNECS_DEBUG_ASSERT(in_world->entity_orders[in_entity] == TNECS_NULL);
    tnecs_entitiesbytype_add(in_world, in_entity, typeflag_new);
    size_t typeflag_new_id = tnecs_typeflagid(in_world, typeflag_new);
    TNECS_DEBUG_ASSERT(in_world->entity_typeflags[in_entity] == typeflag_new);
    TNECS_DEBUG_ASSERT(in_world->entity_orders[in_entity] == in_world->num_entities_bytype[typeflag_new_id] - 1);
    TNECS_DEBUG_ASSERT(in_world->entities_bytype[typeflag_new_id][in_world->entity_orders[in_entity]] == in_entity);
    size_t out = in_world->entity_orders[in_entity];
    return (out);
}

bool tnecs_component_add(struct tnecs_World * in_world, tnecs_component_t in_typeflag) {
    TNECS_DEBUG_PRINTF("tnecs_component_add \n");

    size_t typeflag_id = tnecs_typeflagid(in_world, in_typeflag);
    size_t new_component_num = in_world->num_components_bytype[typeflag_id];
    size_t current_component_id;
    struct tnecs_Components_Array * current_array;
    bool togrow = 0, success = 1;
    for (size_t corder = 0; corder < new_component_num; corder++) {
        current_array = &in_world->components_bytype[typeflag_id][corder];
        togrow |= (++current_array->num_components >= current_array->len_components);
    }
    if (togrow) {
        success &= tnecs_growArray_bytype(in_world, typeflag_id);
    }
    return (success);

}

bool tnecs_component_copy(struct tnecs_World * in_world, tnecs_entity_t in_entity, tnecs_component_t old_typeflag, tnecs_component_t new_typeflag) {
    TNECS_DEBUG_PRINTF("tnecs_component_copy \n");

    TNECS_DEBUG_ASSERT(old_typeflag != TNECS_NULL);
    bool success = 1;
    size_t old_typeflag_id = tnecs_typeflagid(in_world, old_typeflag);
    size_t new_typeflag_id = tnecs_typeflagid(in_world, new_typeflag);
    size_t old_entity_order = in_world->entity_orders[in_entity];
    size_t new_entity_order = in_world->num_entities_bytype[new_typeflag_id];
    TNECS_DEBUG_ASSERT((in_world->components_bytype[new_typeflag_id][0].num_components - 1) == new_entity_order);
    TNECS_DEBUG_ASSERT(old_typeflag != TNECS_NULL);
    size_t old_component_id, new_component_id, component_bytesize;
    struct tnecs_Components_Array * old_array, * new_array;
    tnecs_byte_t * old_component_ptr, * new_component_ptr, * old_component_bytesptr, * new_component_bytesptr;
    for (size_t old_corder = 0; old_corder < in_world->num_components_bytype[old_typeflag_id]; old_corder++) {
        old_component_id = in_world->components_idbytype[old_typeflag_id][old_corder];
        for (size_t new_corder = 0; new_corder < in_world->num_components_bytype[new_typeflag_id]; new_corder++) {
            new_component_id = in_world->components_idbytype[new_typeflag_id][new_corder];
            if (old_component_id == new_component_id) {
                new_array = &in_world->components_bytype[new_typeflag_id][new_corder];
                old_array = &in_world->components_bytype[old_typeflag_id][old_corder];
                component_bytesize = in_world->component_bytesizes[old_component_id];
                TNECS_DEBUG_ASSERT(component_bytesize > 0);
                old_component_bytesptr = (tnecs_byte_t *)(old_array->components);
                success &= (old_component_bytesptr != NULL);
                old_component_ptr = (tnecs_byte_t *)(old_component_bytesptr + (component_bytesize * old_entity_order));
                success &= (old_component_ptr != NULL);
                new_component_bytesptr = (tnecs_byte_t *)(new_array->components);
                success &= (new_component_bytesptr != NULL);
                new_component_ptr = (tnecs_byte_t *)(new_component_bytesptr + (component_bytesize * new_entity_order));
                success &= (new_component_ptr != NULL);
                success &= (new_component_ptr != old_component_ptr);
                success &= (new_component_ptr == memcpy(new_component_ptr, old_component_ptr, component_bytesize));
                break;
            }
        }
    }
    return (success);
}

bool tnecs_component_del(struct tnecs_World * in_world, tnecs_entity_t in_entity, tnecs_component_t old_typeflag) {
    TNECS_DEBUG_PRINTF("tnecs_component_del \n"); // deletes ALL components from componentsbytype

    bool success = 1;
    size_t old_typeflag_id = tnecs_typeflagid(in_world, old_typeflag);
    size_t old_component_num = in_world->num_components_bytype[old_typeflag_id];
    size_t entity_order_old = in_world->entity_orders[in_entity];
    size_t component_order_current, current_component_id, component_bytesize;
    tnecs_component_array_t * old_array;
    tnecs_byte_t * current_component_ptr, * next_component_ptr, * temp_component_ptr;
    for (size_t corder = 0; corder < old_component_num; corder++) {
        current_component_id = in_world->components_idbytype[old_typeflag_id][corder];
        old_array = &in_world->components_bytype[old_typeflag_id][corder];
        temp_component_ptr = (tnecs_byte_t *)old_array->components;
        TNECS_DEBUG_ASSERT(temp_component_ptr != NULL);
        component_bytesize = in_world->component_bytesizes[current_component_id];
        success &= (tnecs_arrdel(temp_component_ptr, entity_order_old, in_world->num_entities_bytype[old_typeflag_id], component_bytesize) != NULL);
        old_array->num_components--;
    }
    return (success);
}

bool tnecs_component_migrate(struct tnecs_World * in_world, tnecs_entity_t in_entity, tnecs_component_t old_typeflag, tnecs_component_t new_typeflag) {
    TNECS_DEBUG_PRINTF("tnecs_component_migrate \n");

    TNECS_DEBUG_ASSERT(old_typeflag == in_world->entity_typeflags[in_entity]);
    bool success = 1;
    success &= tnecs_component_add(in_world, new_typeflag);
    if (old_typeflag > TNECS_NULL) {
        success &= tnecs_component_copy(in_world, in_entity, old_typeflag, new_typeflag);
        success &= tnecs_component_del(in_world, in_entity, old_typeflag);
    }
    return (success);
}

bool tnecs_component_array_new(struct tnecs_World * in_world, size_t num_components, tnecs_component_t in_typeflag) {
    TNECS_DEBUG_PRINTF("tnecs_component_array_new\n");

    struct tnecs_Components_Array * temp_comparray = (struct tnecs_Components_Array *)calloc(num_components, sizeof(struct tnecs_Components_Array));
    tnecs_component_t typeflag_reduced = in_typeflag, typeflag_added = 0, type_toadd, typeflag_id = tnecs_typeflagid(in_world, in_typeflag);
    size_t id_toadd, num_flags = 0;
    while (typeflag_reduced) {
        typeflag_reduced &= (typeflag_reduced - 1);
        type_toadd = (typeflag_reduced + typeflag_added) ^ in_typeflag;
        id_toadd = TNECS_COMPONENT_TYPE2ID(type_toadd);
        TNECS_DEBUG_ASSERT(id_toadd > 0);
        tnecs_component_array_init(in_world, &temp_comparray[num_flags], id_toadd);
        num_flags++;
        typeflag_added += type_toadd;
    }
    in_world->components_bytype[typeflag_id] = temp_comparray;
    return ((typeflag_added == in_typeflag) && (num_flags == num_components));
}

bool tnecs_component_array_init(struct tnecs_World * in_world, struct tnecs_Components_Array * in_array, size_t in_component_id) {
    TNECS_DEBUG_PRINTF("tnecs_component_array_init\n");

    TNECS_DEBUG_ASSERT(in_component_id > 0);
    tnecs_component_t in_type = TNECS_COMPONENT_ID2TYPE(in_component_id);
    TNECS_DEBUG_ASSERT(in_type <= (1 << in_world->num_components));
    size_t bytesize = in_world->component_bytesizes[in_component_id];
    if (bytesize > 0) {
        in_array->type = in_type;
        in_array->num_components = 0;
        in_array->len_components = TNECS_INITIAL_ENTITY_LEN;
        in_array->components = calloc(TNECS_INITIAL_ENTITY_LEN, bytesize);
    }
    return ((in_array->components == NULL) && (in_array->components != NULL));
}

bool tnecs_system_order_switch(struct tnecs_World * in_world, tnecs_phase_t in_phase_id, size_t order1, size_t order2) {
    TNECS_DEBUG_PRINTF("tnecs_system_order_switch\n");

    void (* systems_temp)(struct tnecs_System_Input *);
    TNECS_DEBUG_ASSERT(in_world->num_phases > in_phase_id);
    TNECS_DEBUG_ASSERT(in_world->phases[in_phase_id]);
    TNECS_DEBUG_ASSERT(in_world->num_systems_byphase[in_phase_id] > order1);
    TNECS_DEBUG_ASSERT(in_world->num_systems_byphase[in_phase_id] > order2);
    TNECS_DEBUG_ASSERT(in_world->systems_byphase[in_phase_id][order1]);
    TNECS_DEBUG_ASSERT(in_world->systems_byphase[in_phase_id][order2]);
    systems_temp = in_world->systems_byphase[in_phase_id][order1];
    in_world->systems_byphase[in_phase_id][order1] = in_world->systems_byphase[in_phase_id][order2];
    in_world->systems_byphase[in_phase_id][order2] = systems_temp;
    return ((in_world->systems_byphase[in_phase_id][order1] != NULL) && (in_world->systems_byphase[in_phase_id][order2] != NULL));
}

/************************ UTILITY FUNCTIONS/MACROS ***************************/
size_t tnecs_component_name2id(struct tnecs_World * in_world, const tnecs_str_t * in_name) {
    TNECS_DEBUG_PRINTF("tnecs_component_name2id\n");

    return (tnecs_component_hash2id(in_world, tnecs_hash_djb2(in_name)));
}

size_t tnecs_component_hash2id(struct tnecs_World * in_world, tnecs_hash_t in_hash) {
    TNECS_DEBUG_PRINTF("tnecs_component_hash2id\n");

    size_t out;
    for (size_t i = 0; i < in_world->num_components; i++) {
        if (in_world->component_hashes[i] == in_hash) {
            out = i;
            break;
        }
    }
    return (out);
}

size_t tnecs_component_order_bytype(struct tnecs_World * in_world, size_t in_component_id, tnecs_component_t in_typeflag) {
    TNECS_DEBUG_PRINTF("tnecs_component_order_bytype\n");

    tnecs_component_t in_typeflag_id = tnecs_typeflagid(in_world, in_typeflag);
    return (tnecs_component_order_bytypeid(in_world, in_component_id, in_typeflag_id));
}

size_t tnecs_component_order_bytypeid(struct tnecs_World * in_world, size_t in_component_id, size_t in_typeflag_id) {
    TNECS_DEBUG_PRINTF("tnecs_component_order_bytypeid\n");

    size_t order = TNECS_COMPONENT_CAP;
    for (size_t i = 0; i < in_world->num_components_bytype[in_typeflag_id]; i++) {
        if (in_world->components_idbytype[in_typeflag_id][i] == in_component_id) {
            order = i;
            break;
        }
    }
    return (order);
}

tnecs_component_t tnecs_component_names2typeflag(struct tnecs_World * in_world, size_t argnum, ...) {
    TNECS_DEBUG_PRINTF("tnecs_component_names2typeflag\n");

    va_list ap;
    tnecs_component_t typeflag = 0;
    va_start(ap, argnum);
    for (size_t i = 0; i < argnum; i++) {
        typeflag += in_world->typeflags[tnecs_component_name2id(in_world, va_arg(ap, const tnecs_str_t *))];
    }
    va_end(ap);
    return (typeflag);
}

tnecs_component_t tnecs_component_ids2typeflag(size_t argnum, ...) {
    TNECS_DEBUG_PRINTF("tnecs_component_ids2typeflag\n");

    tnecs_component_t out = 0;
    va_list ap;
    va_start(ap, argnum);
    for (size_t i = 0; i < argnum; i++) {
        out += TNECS_COMPONENT_ID2TYPE(va_arg(ap, size_t));
    }
    va_end(ap);
    return (out);
}

tnecs_component_t tnecs_component_hash2type(struct tnecs_World * in_world, tnecs_hash_t in_hash) {
    TNECS_DEBUG_PRINTF("tnecs_component_hash2type \n");
    return (TNECS_COMPONENT_ID2TYPE(tnecs_component_hash2id(in_world, in_hash)));
}

size_t tnecs_system_name2id(struct tnecs_World * in_world, const tnecs_str_t * in_name) {
    TNECS_DEBUG_PRINTF("tnecs_system_name2id\n");

    tnecs_hash_t in_hash = tnecs_hash_djb2(in_name);
    size_t found = 0;
    for (size_t i = 0; i < in_world->num_systems; i++) {
        if (in_world->system_hashes[i] == in_hash) {
            found = i;
            break;
        }
    }
    return (found);
}

tnecs_component_t tnecs_system_name2typeflag(struct tnecs_World * in_world, const tnecs_str_t * in_name) {
    TNECS_DEBUG_PRINTF("tnecs_system_name2typeflag\n");

    size_t id = tnecs_system_name2id(in_world, in_name);
    return (in_world->system_typeflags[id]);
}

size_t tnecs_typeflagid(struct tnecs_World * in_world, tnecs_component_t in_typeflag) {
    TNECS_DEBUG_PRINTF("tnecs_typeflagid \n");

    size_t id = 0;
    for (size_t i = 0; i < in_world->num_typeflags; i++) {
        if (in_typeflag == in_world->typeflags[i]) {
            id = i;
            break;
        }
    }
    return (id);
}

/***************************** "DYNAMIC" ARRAYS ******************************/
void * tnecs_realloc(void * ptr, size_t old_len, size_t new_len, size_t elem_bytesize) {
    TNECS_DEBUG_PRINTF("tnecs_realloc\n");

    void * temp = (void *)calloc(new_len, elem_bytesize);
    memcpy(temp, ptr, old_len * elem_bytesize);
    TNECS_DEBUG_ASSERT(temp);
    free(ptr);
    return (temp);
}

void * tnecs_arrdel(void * arr, size_t elem, size_t len, size_t bytesize) {
    TNECS_DEBUG_PRINTF("tnecs_arrdel\n");

    void * out;
    if (elem < (len - 1)) {
        out = memcpy(arr + (elem * bytesize), arr + ((elem + 1) * bytesize), bytesize * (len - elem - 1));
    } else {
        out = memset(arr + (elem * bytesize), 0, bytesize);
    }
    return (out);
}

void * tnecs_arrdel_scramble(void * arr, size_t elem, size_t len, size_t bytesize) {
    TNECS_DEBUG_PRINTF("tnecs_arrdel_scramble\n");

    memcpy(arr + (elem * bytesize), arr + ((len - 1) * bytesize), bytesize);
    memset(arr + ((len - 1) * bytesize), 0, bytesize);
    return (arr);
}

bool tnecs_growArray_entity(struct tnecs_World * in_world) {
    TNECS_DEBUG_PRINTF("tnecs_growArray_entity\n");

    size_t old_len = in_world->len_entities;
    bool success = 1;
    in_world->len_entities *= TNECS_ARRAY_GROWTH_FACTOR;
    success &= ((in_world->entity_typeflags = tnecs_realloc(in_world->entity_typeflags, old_len, in_world->len_entities, sizeof(*in_world->entity_typeflags))) != NULL);
    success &= ((in_world->entity_orders = tnecs_realloc(in_world->entity_orders, old_len, in_world->len_entities, sizeof(*in_world->entity_orders))) != NULL);
    success &= ((in_world->entities = tnecs_realloc(in_world->entities, old_len, in_world->len_entities, sizeof(*in_world->entities))) != NULL);
    return (success);
}

bool tnecs_growArray_system(struct tnecs_World * in_world) {
    TNECS_DEBUG_PRINTF("tnecs_growArray_system\n");

    size_t old_len = in_world->len_systems;
    TNECS_DEBUG_ASSERT(old_len > 0);
    bool success = 1;
    in_world->len_systems *= TNECS_ARRAY_GROWTH_FACTOR;
    in_world->len_systems_torun *= TNECS_ARRAY_GROWTH_FACTOR;
    success &= ((in_world->system_names = tnecs_realloc(in_world->system_names, old_len, in_world->len_systems, sizeof(*in_world->system_names))) != NULL);
    success &= ((in_world->systems_torun = tnecs_realloc(in_world->systems_torun, old_len, in_world->len_systems_torun, sizeof(*in_world->systems_torun))) != NULL);
    // systems can be run multiple times.
    success &= ((in_world->system_phases = tnecs_realloc(in_world->system_phases, old_len, in_world->len_systems, sizeof(*in_world->system_phases))) != NULL);
    success &= ((in_world->system_orders = tnecs_realloc(in_world->system_orders, old_len, in_world->len_systems, sizeof(*in_world->system_orders))) != NULL);
    success &= ((in_world->system_exclusive = tnecs_realloc(in_world->system_exclusive, old_len, in_world->len_systems, sizeof(*in_world->system_exclusive))) != NULL);
    success &= ((in_world->system_typeflags = tnecs_realloc(in_world->system_typeflags, old_len, in_world->len_systems, sizeof(*in_world->system_typeflags))) != NULL);
    success &= ((in_world->system_hashes = tnecs_realloc(in_world->system_hashes, old_len, in_world->len_systems, sizeof(*in_world->system_hashes))) != NULL);
    return (success);
}

bool tnecs_growArray_typeflag(struct tnecs_World * in_world) {
    TNECS_DEBUG_PRINTF("tnecs_growArray_typeflag\n");

    size_t old_len = in_world->len_typeflags;
    bool success = 1;
    in_world->len_typeflags *= TNECS_ARRAY_GROWTH_FACTOR;
    success &= ((in_world->typeflags = tnecs_realloc(in_world->typeflags, old_len, in_world->len_typeflags, sizeof(*in_world->typeflags))) != NULL);
    success &= ((in_world->components_bytype = tnecs_realloc(in_world->components_bytype, old_len, in_world->len_typeflags, sizeof(*in_world->components_bytype))) != NULL);
    success &= ((in_world->num_components_bytype = tnecs_realloc(in_world->num_components_bytype, old_len, in_world->len_typeflags, sizeof(*in_world->num_components_bytype))) != NULL);
    success &= ((in_world->entities_bytype = tnecs_realloc(in_world->entities_bytype, old_len, in_world->len_typeflags, sizeof(*in_world->entities_bytype))) != NULL);
    success &= ((in_world->num_entities_bytype = tnecs_realloc(in_world->num_entities_bytype, old_len, in_world->len_typeflags, sizeof(*in_world->num_entities_bytype))) != NULL);
    success &= ((in_world->num_supertype_ids = tnecs_realloc(in_world->num_supertype_ids, old_len, in_world->len_typeflags, sizeof(*in_world->num_supertype_ids))) != NULL);
    success &= ((in_world->len_entities_bytype = tnecs_realloc(in_world->len_entities_bytype, old_len, in_world->len_typeflags, sizeof(*in_world->len_entities_bytype))) != NULL);
    success &= ((in_world->components_idbytype = tnecs_realloc(in_world->components_idbytype, old_len, in_world->len_typeflags, sizeof(*in_world->components_idbytype))) != NULL);
    success &= ((in_world->components_flagbytype = tnecs_realloc(in_world->components_flagbytype, old_len, in_world->len_typeflags, sizeof(*in_world->components_flagbytype))) != NULL);
    success &= ((in_world->components_orderbytype = tnecs_realloc(in_world->components_orderbytype, old_len, in_world->len_typeflags, sizeof(*in_world->components_orderbytype))) != NULL);
    success &= ((in_world->supertype_id_bytype = tnecs_realloc(in_world->supertype_id_bytype, old_len, in_world->len_typeflags, sizeof(*in_world->supertype_id_bytype))) != NULL);
    for (size_t i = old_len; i < in_world->len_typeflags; i++) {
        success &= ((in_world->entities_bytype[i] = calloc(TNECS_INITIAL_ENTITY_LEN, sizeof(**in_world->entities_bytype))) != NULL);
        success &= ((in_world->supertype_id_bytype[i] = calloc(TNECS_COMPONENT_CAP, sizeof(**in_world->supertype_id_bytype))) != NULL);
        in_world->len_entities_bytype[i] = TNECS_INITIAL_ENTITY_LEN;
        in_world->num_entities_bytype[i] = 0;
    }
    return (success);
}

bool tnecs_growArray_phase(struct tnecs_World * in_world) {
    TNECS_DEBUG_PRINTF("tnecs_growArray_phase\n");

    size_t old_len = in_world->len_phases;
    in_world->len_phases *= TNECS_ARRAY_GROWTH_FACTOR;
    bool success = 1;
    success &= ((in_world->systems_byphase = tnecs_realloc(in_world->systems_byphase, old_len, in_world->len_phases, sizeof(*in_world->systems_byphase))) != NULL);
    success &= ((in_world->systems_idbyphase = tnecs_realloc(in_world->systems_idbyphase, old_len, in_world->len_phases, sizeof(*in_world->systems_idbyphase))) != NULL);
    success &= ((in_world->phases = tnecs_realloc(in_world->phases, old_len, in_world->len_phases, sizeof(*in_world->phases))) != NULL);
    success &= ((in_world->len_systems_byphase = tnecs_realloc(in_world->len_systems_byphase, old_len, in_world->len_phases, sizeof(*in_world->len_systems_byphase))) != NULL);
    success &= ((in_world->num_systems_byphase = tnecs_realloc(in_world->num_systems_byphase, old_len, in_world->len_phases, sizeof(*in_world->num_systems_byphase))) != NULL);
    for (size_t i = old_len; i < in_world->len_phases; i++) {
        success &= ((in_world->systems_byphase[i] = calloc(TNECS_INITIAL_PHASE_LEN, sizeof(**in_world->systems_byphase))) != NULL);
        success &= ((in_world->systems_idbyphase[i] = calloc(TNECS_INITIAL_PHASE_LEN, sizeof(**in_world->systems_idbyphase))) != NULL);
        in_world->len_systems_byphase[i] = TNECS_INITIAL_PHASE_LEN;
        in_world->num_systems_byphase[i] = 0;
    }
    return (success);
}

bool tnecs_growArray_bytype(struct tnecs_World * in_world, size_t typeflag_id) {
    TNECS_DEBUG_PRINTF("tnecs_growArray_bytype\n");

    size_t old_len = in_world->len_entities_bytype[typeflag_id];
    bool success = 1;
    if ((success = (old_len > 0))) {
        in_world->len_entities_bytype[typeflag_id] *= TNECS_ARRAY_GROWTH_FACTOR;
        success &= ((in_world->entities_bytype[typeflag_id] = tnecs_realloc(in_world->entities_bytype[typeflag_id], old_len, in_world->len_entities_bytype[typeflag_id], sizeof(*in_world->entities_bytype[typeflag_id]))) != NULL);
        size_t new_component_num = in_world->num_components_bytype[typeflag_id];
        size_t current_component_id;
        tnecs_component_array_t * current_array;
        for (size_t corder = 0; corder < new_component_num; corder++) {
            current_array = &in_world->components_bytype[typeflag_id][corder];
            size_t old_len = current_array->len_components;
            current_component_id = in_world->components_idbytype[typeflag_id][corder];
            TNECS_DEBUG_ASSERT(old_len == current_array->len_components);
            size_t bytesize = in_world->component_bytesizes[current_component_id];
            current_array->len_components *= TNECS_ARRAY_GROWTH_FACTOR;
            success &= ((current_array->components = tnecs_realloc(current_array->components, old_len, current_array->len_components, bytesize)) != NULL);
        }
    }
    return (success);
}

/****************************** STRING HASHING *******************************/
uint64_t tnecs_hash_djb2(const char * str) {
    /* djb2 hashing algorithm by Dan Bernstein.
    * Description: This algorithm (k=33) was first reported by dan bernstein many
    * years ago in comp.lang.c. Another version of this algorithm (now favored by bernstein)
    * uses xor: hash(i) = hash(i - 1) * 33 ^ str[i]; the magic of number 33
    * (why it works better than many other constants, prime or not) has never been adequately explained.
    * [1] https://stackoverflow.com/questions/7666509/hash-function-for-string
    * [2] http://www.cse.yorku.ca/~oz/hash.html */
    TNECS_DEBUG_PRINTF("tnecs_hash_djb2\n");

    uint64_t hash = 5381;
    int32_t str_char;
    while ((str_char = *str++)) {
        hash = ((hash << 5) + hash) + str_char; /* hash * 33 + c */
    }
    return (hash);
}

uint64_t tnecs_hash_sdbm(const char * str) {
    /* sdbm hashing algorithm by Dan Bernstein.
    * Description: This algorithm was created for sdbm (a public-domain
    * reimplementation of ndbm) database library. It was found to do
    * well in scrambling bits, causing better distribution of the
    * keys and fewer splits. It also happens to be a good general hashing
    * function with good distribution. The actual function is
    *hash(i) = hash(i - 1) * 65599 + str[i]; what is included below
    * is the faster version used in gawk. [* there is even a faster,
    * duff-device version] the magic constant 65599 was picked out of
    * thin air while experimenting with different constants, and turns
    * out to be a prime. this is one of the algorithms used in
    * berkeley db (see sleepycat) and elsewhere.
    * [1] https://stackoverflow.com/questions/7666509/hash-function-for-string
    * [2] http://www.cse.yorku.ca/~oz/hash.html */
    TNECS_DEBUG_PRINTF("tnecs_hash_djb2\n");

    uint64_t hash = 0;
    uint32_t str_char;
    while ((str_char = *str++)) {
        hash = str_char + (hash << 6) + (hash << 16) - hash;
    }
    return (hash);
}

/****************************** SET BIT COUNTING *****************************/
size_t setBits_KnR_uint64_t(uint64_t in_flags) {
    // Credits to Kernighan and Ritchie in the C Programming Language
    TNECS_DEBUG_PRINTF("setBits_KnR_uint64_t\n");
    size_t count = 0;
    while (in_flags) {
        in_flags &= (in_flags - 1);
        count++;
    }
    return (count);
}