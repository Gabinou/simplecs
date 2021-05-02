
#include "tnecs.h"

#define STB_DS_IMPLEMENTATION
#include "stb_ds.h" // Should I eliminate this dependency? -> yes


struct tnecs_World * tnecs_init() {
    TNECS_DEBUG_PRINTF("tnecs_init\n");

    struct tnecs_World * tnecs_world = (struct tnecs_World *)calloc(sizeof(struct tnecs_World), 1);
    tnecs_world->entities = NULL;
    arrsetcap(tnecs_world->entities, TNECS_INITIAL_ENTITY_CAP);
    arrput(tnecs_world->entities, TNECS_NULL);

    tnecs_world->typeflags = NULL;
    arrsetcap(tnecs_world->typeflags, TNECS_INITIAL_ENTITY_CAP);
    arrput(tnecs_world->typeflags, TNECS_NULL);

    // tnecs_world->entity_typeflags = NULL;
    // arrsetcap(tnecs_world->entity_typeflags, TNECS_INITIAL_ENTITY_CAP);
    // arrput(tnecs_world->entity_typeflags, TNECS_NULL);
    tnecs_world->entity_typeflags = calloc(TNECS_INITIAL_ENTITY_CAP, sizeof(tnecs_component_t));
    tnecs_world->len_entity_typeflags = TNECS_INITIAL_ENTITY_CAP;
    tnecs_world->num_entity_typeflags = 1;

    tnecs_world->system_typeflags = NULL;
    arrsetcap(tnecs_world->system_typeflags, TNECS_INITIAL_SYSTEM_CAP);
    arrput(tnecs_world->system_typeflags, TNECS_NULL);

    tnecs_world->system_isExclusive = NULL;
    arrsetcap(tnecs_world->system_isExclusive, TNECS_INITIAL_SYSTEM_CAP);
    arrput(tnecs_world->system_isExclusive, TNECS_NULL);

    tnecs_world->component_hashes[TNECS_NULL] = TNECS_NULL;

    tnecs_world->system_hashes = NULL;
    arrsetcap(tnecs_world->system_hashes, TNECS_INITIAL_SYSTEM_CAP);
    arrput(tnecs_world->system_hashes, TNECS_NULL);

    tnecs_world->entities_bytype = NULL;
    arrsetcap(tnecs_world->entities_bytype, TNECS_INITIAL_SYSTEM_CAP);
    arrsetlen(tnecs_world->num_entitiesbytype, TNECS_INITIAL_SYSTEM_CAP);
    tnecs_world->num_componentsbytype = NULL;
    arrsetcap(tnecs_world->num_componentsbytype, TNECS_INITIAL_SYSTEM_CAP);
    for (size_t i = 0 ; i < TNECS_INITIAL_SYSTEM_CAP; i++) {
        tnecs_world->num_entitiesbytype[i] = 0;
        tnecs_world->num_componentsbytype[i] = 0;
    }
    arrput(tnecs_world->entities_bytype, NULL);
    arrput(tnecs_world->entities_bytype[TNECS_NULL], TNECS_NULL);
    tnecs_world->num_entitiesbytype[TNECS_NULL]++;

    tnecs_world->component_idbytype = NULL;
    arrsetcap(tnecs_world->component_idbytype, TNECS_INITIAL_SYSTEM_CAP);
    arrput(tnecs_world->component_idbytype, NULL);

    tnecs_world->component_flagbytype = NULL;
    arrsetcap(tnecs_world->component_flagbytype, TNECS_INITIAL_SYSTEM_CAP);
    arrput(tnecs_world->component_flagbytype, NULL);

    tnecs_world->num_entitiesbytype = NULL;
    arrsetcap(tnecs_world->num_entitiesbytype, TNECS_INITIAL_SYSTEM_CAP);

    tnecs_world->num_components = TNECS_ID_START;
    tnecs_world->num_systems = TNECS_ID_START;
    tnecs_world->num_typeflags = TNECS_ID_START;

    tnecs_world->components_bytype = NULL;
    arrsetcap(tnecs_world->components_bytype, TNECS_INITIAL_SYSTEM_CAP);
    arrput(tnecs_world->components_bytype, NULL);

    tnecs_world->systems = NULL;
    arrsetcap(tnecs_world->systems, TNECS_INITIAL_SYSTEM_CAP);
    arrput(tnecs_world->systems, NULL);

    tnecs_world->next_entity_id = TNECS_ID_START;

    return (tnecs_world);
}

tnecs_entity_t tnecs_new_entity(struct tnecs_World * in_world) {
    TNECS_DEBUG_PRINTF("tnecs_new_entity\n");

    tnecs_entity_t out = TNECS_NULL;
    tnecs_component_t component_flag;
    while ((out == TNECS_NULL) && (in_world->num_opened_entity_ids > 0)) {
        out = in_world->opened_entity_ids[--in_world->num_opened_entity_ids];
        in_world->opened_entity_ids[in_world->num_opened_entity_ids] = TNECS_NULL;
    }
    if (out == TNECS_NULL) {
        out = in_world->next_entity_id++;
    }
    TNECS_DEBUG_ASSERT(out != TNECS_NULL);
    arrput(in_world->entities, out);
    tnecs_entity_typeflag_add(in_world, out, TNECS_NOCOMPONENT_TYPEFLAG);

    // arrput(in_world->entity_typeflags, TNECS_NOCOMPONENT_TYPEFLAG);
    tnecs_entitiesbytype_add(in_world, out, TNECS_NOCOMPONENT_TYPEFLAG);
    return (out);
}

void * tnecs_entity_get_component(struct tnecs_World * in_world, tnecs_entity_t in_entity_id, tnecs_component_t in_component_id) {
    TNECS_DEBUG_PRINTF("tnecs_entity_get_component\n");

    tnecs_component_t component_flag = TNECS_COMPONENT_ID2TYPEFLAG(in_component_id);
    tnecs_component_t entity_typeflag = TNECS_ENTITY_TYPEFLAG(in_world, in_entity_id);

    size_t typeflag_id = tnecs_typeflagid(in_world, entity_typeflag);
    size_t component_order = tnecs_componentid_order_bytype(in_world, in_component_id, entity_typeflag);
    size_t entity_order = tnecs_entity_order_bytypeid(in_world, in_entity_id, typeflag_id);
    size_t bytesize = in_world->component_bytesizes[in_component_id];
    struct tnecs_Components_Array * comp_array = &in_world->components_bytype[typeflag_id][component_order];
    tnecs_byte_t * temp_component_bytesptr = (tnecs_byte_t *)(comp_array->components);
    printf("entity_order %d \n", entity_order);
    printf("(comp_array->components == NULL) %d \n", (comp_array->components == NULL));
    void * out_component = temp_component_bytesptr + (bytesize * entity_order);
    return (out_component);
}

void tnecs_component_array_init(struct tnecs_World * in_world, struct tnecs_Components_Array * in_array, size_t in_component_id) {
    tnecs_component_t in_type = TNECS_COMPONENT_ID2TYPEFLAG(in_component_id);
    size_t bytesize = in_world->component_bytesizes[in_component_id];
    in_array->type = in_type;
    in_array->num_components = 0;
    in_array->len_components = TNECS_INITIAL_ENTITY_CAP;
    in_array->components = calloc(TNECS_INITIAL_ENTITY_CAP, bytesize);
}


void tnecs_component_array_realloc(struct tnecs_World * in_world, tnecs_component_t entity_typeflag, tnecs_component_t in_component_id) {
    TNECS_DEBUG_PRINTF("tnecs_component_array_realloc\n");

    size_t component_order = tnecs_componentid_order_bytype(in_world, in_component_id, entity_typeflag);
    struct tnecs_Components_Array * current_array = &in_world->components_bytype[entity_typeflag][component_order];
    size_t old_len = current_array->len_components;
    if (old_len < TNECS_INITIAL_ENTITY_CAP) {
        current_array->len_components = TNECS_COMPONENT_ALLOCBLOCK * 2;
    } else {
        current_array->len_components += TNECS_COMPONENT_ALLOCBLOCK;
    }
    void * temp = calloc(current_array->len_components, in_world->component_bytesizes[in_component_id]);

    memcpy(temp, current_array->components, old_len * in_world->component_bytesizes[in_component_id]);
    free(current_array->components);
    current_array->components = temp;
}

void * tnecs_realloc(void * ptr, size_t old_len, size_t new_len, size_t elem_bytesize) {
    void * temp = calloc(new_len, elem_bytesize);
    memcpy(temp, ptr, old_len * elem_bytesize);
    free(ptr);
    return (temp);
}


void tnecs_entity_typeflag_add(struct tnecs_World * in_world, tnecs_entity_t in_entity, tnecs_component_t in_typeflag) {
    TNECS_DEBUG_PRINTF("tnecs_entitiesbytype_add\n");

    if (in_entity > in_world->len_entity_typeflags) {
        size_t old_len = in_world->len_entity_typeflags;
        size_t bytesize = sizeof(in_world->entity_typeflags[0]);

        in_world->len_entity_typeflags = in_entity + TNECS_INITIAL_ENTITY_CAP;
        in_world->entity_typeflags = tnecs_realloc(in_world->entity_typeflags, old_len, in_world->len_entity_typeflags, bytesize);
    }

    if (++in_world->num_entity_typeflags < in_world->len_entity_typeflags) {
        size_t old_len = in_world->len_entity_typeflags;
        size_t bytesize = sizeof(in_world->entity_typeflags[0]);

        in_world->len_entity_typeflags += TNECS_INITIAL_ENTITY_CAP;
        in_world->entity_typeflags = tnecs_realloc(in_world->entity_typeflags, old_len, in_world->len_entity_typeflags, bytesize);
    }

    in_world->entity_typeflags[in_entity] = in_typeflag;
}

size_t tnecs_entitiesbytype_add(struct tnecs_World * in_world, tnecs_entity_t in_entity, tnecs_component_t typeflag_new) {
    TNECS_DEBUG_PRINTF("tnecs_entitiesbytype_add\n");

    size_t typeflag_id_new = tnecs_typeflagid(in_world, typeflag_new);
    arrput(in_world->entities_bytype[typeflag_id_new], in_entity);
    return (in_world->num_entitiesbytype[typeflag_id_new]++);
}

void tnecs_entitiesbytype_del(struct tnecs_World * in_world, tnecs_entity_t in_entity, tnecs_component_t typeflag_old) {
    TNECS_DEBUG_PRINTF("tnecs_entitiesbytype_del\n");
    size_t entity_order_old = tnecs_entity_order_bytype(in_world, in_entity, typeflag_old);
    size_t typeflag_id_old = tnecs_typeflagid(in_world, typeflag_old);

    TNECS_DEBUG_ASSERT(in_world->entities_bytype[typeflag_id_old][entity_order_old] == in_entity);
    arrdel(in_world->entities_bytype[typeflag_id_old], entity_order_old);
    in_world->num_entitiesbytype[typeflag_id_old]--;
}

size_t tnecs_entitiesbytype_migrate(struct tnecs_World * in_world, tnecs_entity_t in_entity, tnecs_component_t typeflag_new) {
    TNECS_DEBUG_PRINTF("tnecs_entitiesbytype_migrate\n");
    tnecs_component_t typeflag_old = in_world->entity_typeflags[in_entity];
    tnecs_entitiesbytype_del(in_world, in_entity, typeflag_old);
    return (tnecs_entitiesbytype_add(in_world, in_entity, typeflag_new));
}

void tnecs_entity_add_components(struct tnecs_World * in_world, tnecs_entity_t in_entity, size_t num_components, tnecs_component_t typeflag_toadd, bool isNew) {
    TNECS_DEBUG_PRINTF("tnecs_entity_add_components\n");

    tnecs_component_t typeflag_old = in_world->entity_typeflags[in_entity];
    tnecs_component_t typeflag_new = typeflag_toadd + typeflag_old;
    size_t entity_order_old = tnecs_entity_order_bytype(in_world, in_entity, typeflag_old);

    // 1- Checks if the new entity_typeflag exists, if not create empty component array
    if (isNew) {
        tnecs_new_typeflag(in_world, num_components, typeflag_new);
    }
    // 2- Migrate entity in entities_bytype old_typeflag->typeflag_new, old_order->new_order
    size_t entity_order_new = tnecs_entitiesbytype_migrate(in_world, in_entity, typeflag_new);
    tnecs_component_t typeflag_id_new = tnecs_typeflagid(in_world, typeflag_new);

    // 3- Migrate components_bytype old_typeflag->typeflag_new
    tnecs_component_migrate(in_world, in_entity, entity_order_new, typeflag_new);
}

void tnecs_new_component_array(struct tnecs_World * in_world, size_t num_components, tnecs_component_t in_typeflag) {
    TNECS_DEBUG_PRINTF("tnecs_new_component_array\n");
    // assumes new typeflag was added on top of world->typeflags

    struct tnecs_Components_Array * temp_comparray = (struct tnecs_Components_Array *)calloc(num_components, sizeof(struct tnecs_Components_Array));

    tnecs_component_t typeflag_reduced = in_typeflag;
    tnecs_component_t typeflag_added = 0;
    tnecs_component_t type_toadd;
    size_t id_toadd;
    size_t i = 0;
    while (typeflag_reduced) {
        typeflag_reduced &= (typeflag_reduced - 1);
        type_toadd = (typeflag_reduced + typeflag_added) ^ in_typeflag;
        id_toadd = TNECS_COMPONENT_TYPE2ID(type_toadd);
        tnecs_component_array_init(in_world, &temp_comparray[i], id_toadd);

        i++;
        typeflag_added += type_toadd;
    }
    TNECS_DEBUG_ASSERT(i == num_components);
    size_t typeflag_id = TNECS_TYPEFLAGID(world, in_typeflag);
    arrput(in_world->components_bytype, temp_comparray);
    arrput(in_world->entities_bytype, NULL);
    arrput(in_world->num_componentsbytype, num_components);
    arrput(in_world->num_entitiesbytype, 0);
}

size_t tnecs_new_typeflag(struct tnecs_World * in_world, size_t num_components, tnecs_component_t typeflag_new) {
    TNECS_DEBUG_PRINTF("tnecs_new_typeflag\n");

    // outputs 0 is typeflag is new, its index if not
    size_t typeflag_id = 0;
    for (size_t i = 0 ; i < in_world->num_typeflags; i++) {
        if (typeflag_new == in_world->typeflags[i]) {
            typeflag_id = i;
            break;
        }
    }
    if (!typeflag_id) {
        // 1- Add new components_bytype at [typeflag_id]
        arrput(in_world->typeflags, typeflag_new);
        in_world->num_typeflags++;
        size_t new_typeflag_id = tnecs_typeflagid(in_world, typeflag_new);
        TNECS_DEBUG_ASSERT(new_typeflag_id == (in_world->num_typeflags - 1));

        // 2- Add arrays to components_bytype[typeflag_id] for each component
        tnecs_new_component_array(in_world, num_components, typeflag_new);

        // 3- Add all components to component_idbytype and component_flagbytype
        arrput(in_world->component_idbytype, NULL);
        arrput(in_world->component_flagbytype, NULL);
        tnecs_component_t component_id_toadd, component_type_toadd;
        tnecs_component_t typeflag_reduced = typeflag_new;
        tnecs_component_t typeflag_added = 0;

        while (typeflag_reduced) {
            typeflag_reduced &= (typeflag_reduced - 1);
            component_type_toadd = (typeflag_reduced + typeflag_added) ^ typeflag_new;
            component_id_toadd = TNECS_COMPONENT_TYPE2ID(component_type_toadd);
            arrput(in_world->component_idbytype[new_typeflag_id], component_id_toadd);
            arrput(in_world->component_flagbytype[new_typeflag_id], component_type_toadd);
            typeflag_added += component_type_toadd;
        }
    }
    return (typeflag_id);
}

size_t tnecs_component_hash2id(struct tnecs_World * in_world, uint64_t in_hash) {
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

size_t tnecs_component_name2id(struct tnecs_World * in_world, const unsigned char * in_name) {
    TNECS_DEBUG_PRINTF("tnecs_component_name2id\n");

    return (tnecs_component_hash2id(in_world, hash_djb2(in_name)));
}

tnecs_component_t tnecs_names2typeflag(struct tnecs_World * in_world, size_t argnum, ...) {
    TNECS_DEBUG_PRINTF("tnecs_names2typeflag\n");

    tnecs_component_t out = 0;
    va_list ap;
    va_start(ap, argnum);
    uint64_t temp_hash;
    for (size_t i = 0; i < argnum; i++) {
        temp_hash = hash_djb2(va_arg(ap, const unsigned char *));
        for (size_t j = 0; j < in_world->num_components; j++) {
            if (in_world->component_hashes[j] == temp_hash) {
                out += TNECS_COMPONENT_ID2TYPEFLAG(j);
                break;
            }
        }
    }
    va_end(ap);
    return (out);
}

tnecs_component_t tnecs_component_ids2typeflag(size_t argnum, ...) {
    TNECS_DEBUG_PRINTF("tnecs_component_ids2typeflag\n");

    tnecs_component_t out = 0;
    va_list ap;
    va_start(ap, argnum);
    for (size_t i = 0; i < argnum; i++) {
        out += TNECS_COMPONENT_ID2TYPEFLAG(va_arg(ap, size_t));
    }
    va_end(ap);
    return (out);
}

tnecs_component_t tnecs_component_hash2typeflag(struct tnecs_World * in_world, uint64_t in_hash) {
    tnecs_component_t out = TNECS_NULL;
    for (size_t i = 0; i < in_world->num_components; i++) {
        if (in_world->component_hashes[i] == in_hash) {
            out = TNECS_COMPONENT_ID2TYPE(i);
        }
    }
    return (out);
}


tnecs_entity_t tnecs_new_entity_wcomponents(struct tnecs_World * in_world, size_t argnum, ...) {
    TNECS_DEBUG_PRINTF("tnecs_new_entity_wcomponents \n");

    va_list ap;
    va_start(ap, argnum);
    tnecs_component_t typeflag = 0;
    uint64_t current_hash;
    for (size_t i = 0; i < argnum; i++) {
        current_hash = va_arg(ap, uint64_t);
        TNECS_DEBUG_PRINTF("Current hash %llu\n", current_hash);
        typeflag += tnecs_component_hash2typeflag(in_world, current_hash);

    }
    va_end(ap);
    tnecs_entity_t new_entity = tnecs_new_entity(in_world);
    size_t typeflag_id = tnecs_new_typeflag(in_world, argnum, typeflag);
    tnecs_entity_typeflag_add(in_world, new_entity, typeflag);
    tnecs_component_add(in_world, typeflag);
    tnecs_entitiesbytype_add(in_world, new_entity, typeflag);
    return (new_entity);
}

void tnecs_entity_destroy(struct tnecs_World * in_world, tnecs_entity_t in_entity) {
    TNECS_DEBUG_PRINTF("tnecs_entity_destroy \n");

    tnecs_component_t previous_flag = in_world->entity_typeflags[in_entity];
    for (size_t i = 0 ; i < in_world->num_systems; i++) {
        if (previous_flag == in_world->system_typeflags[i]) {
            for (size_t j = 0 ; j < in_world->num_entitiesbytype[i]; j++) {
                if (in_world->entities_bytype[i][j] == in_entity) {
                    arrdel(in_world->entities_bytype[i], j);
                    break;
                }
            }
        }

        in_world->entity_typeflags[in_entity] = 0;
        if (in_world->num_opened_entity_ids < TNECS_OPEN_IDS_BUFFER) {
            in_world->opened_entity_ids[in_world->num_opened_entity_ids++] = in_entity;
        }
    }
}

void tnecs_register_component(struct tnecs_World * in_world, uint64_t in_hash, size_t in_bytesize) {
    TNECS_DEBUG_PRINTF("tnecs_register_component\n");

    if (in_world->num_components < TNECS_COMPONENT_CAP) {
        in_world->component_hashes[in_world->num_components] = in_hash;
        tnecs_component_t new_component_flag =  TNECS_COMPONENT_ID2TYPEFLAG(in_world->num_components);
        size_t typeflag_id = tnecs_new_typeflag(in_world, 1, new_component_flag);
        in_world->component_bytesizes[in_world->num_components] = in_bytesize;
        in_world->num_components++;
    } else {
        printf("TNECS ERROR: Cannot register more than 63 components");
    }
}

void tnecs_register_system(struct tnecs_World * in_world, uint64_t in_hash, void (* in_system)(struct tnecs_System_Input), uint8_t in_run_phase, bool isExclusive, size_t num_components, tnecs_component_t components_typeflag) {
    TNECS_DEBUG_PRINTF("tnecs_register_system\n");

    arrput(in_world->system_isExclusive, isExclusive);
    arrput(in_world->system_phase, in_run_phase);
    arrput(in_world->system_hashes, in_hash);
    arrput(in_world->system_typeflags, components_typeflag);
    arrput(in_world->systems, in_system);
    arrput(in_world->num_componentsbytype, num_components);

    size_t typeflag_id = tnecs_new_typeflag(in_world, num_components, components_typeflag);
    in_world->num_systems++;
}

void tnecs_component_add(struct tnecs_World * in_world, tnecs_component_t new_typeflag) {
    TNECS_DEBUG_PRINTF("tnecs_component_add \n");

    // 1- Check if component array has enough room
    // room should always be set to 0
    size_t new_typeflag_id = tnecs_type_id(in_world->system_typeflags, in_world->num_typeflags, new_typeflag);
    size_t new_component_num = in_world->num_componentsbytype[new_typeflag_id];
    struct tnecs_Components_Array * current_array;
    tnecs_component_t component_id;
    for (size_t corder = 0; corder < new_component_num; corder++) {
        current_array = &in_world->components_bytype[new_typeflag_id][corder];
        component_id = in_world->component_idbytype[new_typeflag_id][corder];
        if (++current_array->num_components >= current_array->len_components) {
            tnecs_component_array_realloc(in_world, new_typeflag, component_id);
        }
    }
}


void tnecs_component_copy(struct tnecs_World * in_world, tnecs_entity_t in_entity, tnecs_component_t old_flag, tnecs_component_t new_flag) {
    TNECS_DEBUG_PRINTF("tnecs_component_copy \n");

    // For use by componentsbytype_migrate
    size_t old_type_id = tnecs_type_id(in_world->system_typeflags, in_world->num_typeflags, old_flag);
    size_t new_type_id = tnecs_type_id(in_world->system_typeflags, in_world->num_typeflags, new_flag);
    size_t old_component_num = in_world->num_componentsbytype[old_type_id];
    size_t new_component_num = in_world->num_componentsbytype[new_type_id];
    size_t old_component_id, new_component_id;
    size_t old_entity_order = tnecs_entity_order_bytypeid(in_world, in_entity, old_type_id);
    size_t new_entity_order = in_world->num_entitiesbytype[new_type_id]++;
    size_t component_bytesize;
    tnecs_byte_t * old_component_ptr, * new_component_ptr;
    tnecs_byte_t * temp_component_bytesptr;
    printf("HERE1\n");
    for (size_t old_corder = 0; old_corder < in_world->num_componentsbytype[old_type_id]; old_corder++) {
        printf("HERE2\n");
        printf("old_type_id %d\n", old_type_id);
        printf("old_corder %d\n", old_corder);
        old_component_id = in_world->component_idbytype[old_type_id][old_corder];
        printf("HERE3\n");
        for (size_t new_corder = 0; new_corder < in_world->num_componentsbytype[new_type_id]; new_corder++) {
            new_component_id = in_world->component_idbytype[new_type_id][new_corder];
            if (old_component_id == new_component_id) {
                printf("HERE2\n");
                component_bytesize = in_world->component_bytesizes[old_component_id];
                printf("HERE3\n");
                temp_component_bytesptr = (tnecs_byte_t *)(in_world->components_bytype[old_type_id][old_corder].components);
                printf("HERE4\n");
                old_component_ptr = temp_component_bytesptr + (component_bytesize * old_entity_order);
                printf("HERE5\n");
                temp_component_bytesptr = (tnecs_byte_t *)(in_world->components_bytype[new_type_id][new_corder].components);
                printf("HERE6\n");
                new_component_ptr = (tnecs_byte_t *)(temp_component_bytesptr + (component_bytesize * new_entity_order));
                printf("HERE7\n");
                memcpy(new_component_ptr, old_component_ptr, component_bytesize);
                break;
            }
        }
    }
    // in_world->num_entitiesbytype[old_type_id]++;
}

void tnecs_component_del(struct tnecs_World * in_world, tnecs_entity_t in_entity, tnecs_component_t old_typeflag) {
    TNECS_DEBUG_PRINTF("tnecs_component_del \n");

    // for input entity, delete ALL components from componentsbytype
    // DOES NOT TOUCHE ENTITIES_BY TYPE OR ANYTHING.
    size_t old_typeflag_id = tnecs_type_id(in_world->system_typeflags, in_world->num_typeflags, old_typeflag);
    size_t old_component_num = in_world->num_componentsbytype[old_typeflag_id];
    size_t entity_order_old = tnecs_entity_order_bytype(in_world, in_entity, old_typeflag);
    size_t component_order_current;
    size_t current_component_id;
    size_t component_bytesize;
    tnecs_byte_t * current_component_ptr, * next_component_ptr;
    tnecs_byte_t * temp_component_ptr;
    for (size_t corder = 0; corder < old_component_num; corder++) {
        current_component_id = in_world->component_idbytype[old_typeflag_id][corder];
        temp_component_ptr = (tnecs_byte_t *)in_world->components_bytype[old_typeflag_id][corder].components;
        component_bytesize = in_world->component_bytesizes[current_component_id];

        for (size_t eorder = entity_order_old; eorder < (in_world->num_entitiesbytype[old_typeflag_id] - 1); eorder++) {
            current_component_ptr = temp_component_ptr + (component_bytesize * eorder);
            next_component_ptr = temp_component_ptr + (component_bytesize * (eorder + 1));
            memcpy(current_component_ptr, next_component_ptr, component_bytesize);
        }
        memset(next_component_ptr, 0, component_bytesize);

    }
    // in_world->num_entitiesbytype[old_typeflag]--;
}

bool tnecs_component_migrate(struct tnecs_World * in_world, tnecs_entity_t in_entity, size_t entity_order_new, tnecs_component_t new_flag) {
    TNECS_DEBUG_PRINTF("tnecs_component_migrate \n");

    // Migrates components associated with in_entity: old_flag -> new_flag
    //      -> copies attached components in componentsbytype old_flag -> new_flag
    //      -> deletes attachated components old_flag, reorders.
    //      -> deletes in_entity from entities_by_type of old_flag, reorders.
    tnecs_component_t old_flag = in_world->entity_typeflags[in_entity];
    if (old_flag > TNECS_NULL) {
        tnecs_component_copy(in_world, in_entity, old_flag, new_flag);
        tnecs_component_del(in_world, in_entity, old_flag);
    } else {
        tnecs_component_add(in_world, new_flag);
    }
    tnecs_entitiesbytype_migrate(in_world, in_entity, new_flag);
}

size_t tnecs_type_id(tnecs_component_t * in_typelist, size_t len, tnecs_component_t in_flag) {
    TNECS_DEBUG_PRINTF("tnecs_type_id \n");

    size_t found = TNECS_NULL;
    for (size_t i = 0; i < len; i++) {
        if (in_typelist[i] == in_flag) {
            found = i;
            break;
        }
    }
    return (found);
}

size_t tnecs_typeflagid(struct tnecs_World * in_world, tnecs_component_t in_typeflag) {
    TNECS_DEBUG_PRINTF("tnecs_typeflagid \n");

    size_t id = 0;
    for (size_t i = 0; i <= in_world->num_typeflags; i++) {
        if (in_typeflag == in_world->typeflags[i]) {
            id = i;
            break;
        }
    }
    return (id);
}


size_t tnecs_issubtype(tnecs_component_t * in_typelist, size_t len, tnecs_component_t in_flag) {
    // returns position of subtype from in_typelist
    TNECS_DEBUG_PRINTF("tnecs_issubtype\n");

    size_t found = 0;
    for (size_t i = 0; i < len; i++) {
        if ((in_typelist[i] & in_flag) == in_flag) {
            found = i;
            break;
        }
    }
    return (found);
}

size_t tnecs_system_hash2id(struct tnecs_World * in_world, uint64_t in_hash) {
    TNECS_DEBUG_PRINTF("tnecs_system_hash2id\n");

    size_t found = 0;
    for (size_t i = 0; i < in_world->num_systems; i++) {
        if (in_world->system_hashes[i] == in_hash) {
            found = i;
            break;
        }
    }
    return (found);
}

tnecs_component_t tnecs_component_hash2type(struct tnecs_World * in_world, uint64_t in_hash) {
    TNECS_DEBUG_PRINTF("tnecs_component_hash2type \n");
    return (TNECS_COMPONENT_ID2TYPEFLAG(tnecs_system_hash2id(in_world, in_hash)));
}

size_t tnecs_system_name2id(struct tnecs_World * in_world, const unsigned char * in_name) {
    TNECS_DEBUG_PRINTF("tnecs_system_name2id\n");
    return (tnecs_system_hash2id(in_world, hash_djb2(in_name)));
}

tnecs_component_t tnecs_system_name2typeflag(struct tnecs_World * in_world, const unsigned char * in_name) {
    TNECS_DEBUG_PRINTF("tnecs_system_name2typeflag\n");
    size_t id = tnecs_system_hash2id(in_world, hash_djb2(in_name));
    return (in_world->system_typeflags[id]);
}

tnecs_component_t tnecs_component_names2typeflag(struct tnecs_World * in_world, size_t argnum, ...) {
    TNECS_DEBUG_PRINTF("tnecs_component_names2typeflag\n");

    va_list ap;
    tnecs_component_t typeflag = 0;
    va_start(ap, argnum);
    for (size_t i = 0; i < argnum; i++) {
        typeflag += in_world->typeflags[tnecs_component_name2id(in_world, va_arg(ap, const unsigned char *))];
    }
    va_end(ap);
    return (typeflag);
}

size_t tnecs_componentflag_order_bytype(struct tnecs_World * in_world, tnecs_component_t in_component_flag, tnecs_component_t in_typeflag) {
    TNECS_DEBUG_PRINTF("tnecs_componentflag_order_bytype\n");

    size_t order = TNECS_COMPONENT_CAP;
    // tnecs_component_t in_typeflag_id = tnecs_component_flag2id(in_world, in_com);
    // for (size_t i = 0; i < in_world->num_componentsbytype[in_typeflag_id]; i++) {
    //     if (in_world->component_flagbytype[in_typeflag_id][i] == in_component_flag) {
    //         order = i;
    //         break;
    //     }
    // }
    return (order);
}

size_t tnecs_componentid_order_bytype(struct tnecs_World * in_world, size_t in_component_id, tnecs_component_t in_typeflag) {
    TNECS_DEBUG_PRINTF("tnecs_componentid_order_bytype\n");

    size_t order = TNECS_COMPONENT_CAP;
    tnecs_component_t in_typeflag_id = tnecs_typeflagid(in_world, in_typeflag);
    for (size_t i = 0; i < in_world->num_componentsbytype[in_typeflag_id]; i++) {
        if (in_world->component_idbytype[in_typeflag_id][i] == in_component_id) {
            order = i;
            break;
        }
    }
    return (order);
}

size_t tnecs_entity_order_bytype(struct tnecs_World * in_world, tnecs_entity_t in_entity, tnecs_component_t in_typeflag) {
    TNECS_DEBUG_PRINTF("tnecs_entity_order_bytype\n");

    tnecs_component_t in_typeflag_id = tnecs_typeflagid(in_world, in_typeflag);
    return (tnecs_entity_order_bytypeid(in_world, in_entity, in_typeflag_id));
}

size_t tnecs_entity_order_bytypeid(struct tnecs_World * in_world, tnecs_entity_t in_entity, tnecs_component_t in_typeflag_id) {
    TNECS_DEBUG_PRINTF("tnecs_entity_order_bytypeid\n");

    size_t order = in_world->next_entity_id;
    for (size_t i = 0; i < in_world->num_entitiesbytype[in_typeflag_id]; i++) {
        // TNECS_DEBUG_PRINTF("i %d\n", i);
        if (in_world->entities_bytype[in_typeflag_id][i] == in_entity) {
            order = i;
            break;
        }
    }
    return (order);
}

size_t tnecs_system_order_byphase(struct tnecs_World * in_world, size_t in_system_id, uint8_t in_phase) {
    TNECS_DEBUG_PRINTF("tnecs_system_order_byphase\n");

    // SYStems need to be stored BY PHASE also.
    // size_t order = in_world->num_systems;
    // for (size_t i = 0; i < in_world->num_systemssbyphase[in_phase]; i++) {
    //     if (in_world->system_idbyphase[in_phase][i] == in_system_id) {
    //         order = i;
    //         break;
    //     }
    // }
    // return (order);
}


// STRING HASHING
uint64_t hash_djb2(const unsigned char * str) {
    /* djb2 hashing algorithm by Dan Bernstein.
    * Description: This algorithm (k=33) was first reported by dan bernstein many
    * years ago in comp.lang.c. Another version of this algorithm (now favored by bernstein)
    * uses xor: hash(i) = hash(i - 1) * 33 ^ str[i]; the magic of number 33
    * (why it works better than many other constants, prime or not) has never been adequately explained.
    * [1] https://stackoverflow.com/questions/7666509/hash-function-for-string
    * [2] http://www.cse.yorku.ca/~oz/hash.html */
    TNECS_DEBUG_PRINTF("hash_djb2\n");

    uint64_t hash = 5381;
    int32_t str_char;
    while (str_char = *str++) {
        hash = ((hash << 5) + hash) + str_char; /* hash * 33 + c */
    }
    return (hash);
}

uint64_t hash_sdbm(const unsigned char * str) {
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
    TNECS_DEBUG_PRINTF("hash_djb2\n");

    uint64_t hash = 0;
    uint32_t str_char;
    while (str_char = *str++) {
        hash = str_char + (hash << 6) + (hash << 16) - hash;
    }
    return (hash);
}

// SET BIT COUNTING
int8_t setBits_KnR_uint64_t(uint64_t in_flags) {
    // Credits to Kernighan and Ritchie in the C Programming Language
    // should output -1 on error
    uint64_t count = 0;
    while (in_flags) {
        in_flags &= (in_flags - 1);
        count++;
    }
    return (count);
}

int8_t setBits_first(uint64_t in_flag) {
    // untested. Broken.
    int8_t index = 0;
    // while (((in_flag >> 1) != 0) && (index < 65)) {
    //     index++
    // }
    // if (index == 65) {
    //     index = -1;
    // }
    // return (index);
}