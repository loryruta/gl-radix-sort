#version 460

#define THREAD_IDX        gl_LocalInvocationIndex
#define THREADS_NUM       64
#define THREAD_BLOCK_IDX  (gl_WorkGroupID.x + gl_NumWorkGroups.x * (gl_WorkGroupID.y + gl_NumWorkGroups.z * gl_WorkGroupID.z))
#define THREAD_BLOCKS_NUM (gl_NumWorkGroups.x * gl_NumWorkGroups.y * gl_NumWorkGroups.z)
#define ITEMS_NUM         4

#define BITSET_NUM        4
#define BITSET_SIZE       uint(exp2(BITSET_NUM))

layout(local_size_x = THREADS_NUM, local_size_y = 1, local_size_z = 1) in;

layout(std430, binding = 0) buffer ssbo_key           { uint b_key_buf[];  };
layout(std430, binding = 1) buffer ssbo_count_buf     { uint b_count_buf[]; }; // [THREAD_BLOCKS_NUM * BITSET_SIZE]
layout(std430, binding = 2) buffer ssbo_tot_count_buf { uint b_tot_count_buf[BITSET_SIZE]; };

uniform uint u_arr_len;
uniform uint u_bitset_idx;

uint to_partition_radixes_offsets_idx(uint radix, uint thread_block_idx)
{
    uint pow_of_2_thread_blocks_num = uint(exp2(ceil(log2(THREAD_BLOCKS_NUM))));
    return radix * pow_of_2_thread_blocks_num + thread_block_idx;
}

uint to_loc_idx(uint item_idx, uint thread_idx)
{
    return (thread_idx * ITEMS_NUM + item_idx);
}

uint to_key_idx(uint item_idx, uint thread_idx, uint thread_block_idx)
{
    return (thread_block_idx * ITEMS_NUM * THREADS_NUM) + (thread_idx * ITEMS_NUM) + item_idx;
}

void main()
{
    for (uint item_idx = 0; item_idx < ITEMS_NUM; item_idx++)
    {
        uint key_idx = to_key_idx(item_idx, THREAD_IDX, THREAD_BLOCK_IDX);
        if (key_idx >= u_arr_len) {
            continue;
        }

        uint bitset_mask = (BITSET_SIZE - 1) << (BITSET_NUM * u_bitset_idx);
        uint rad = (b_key_buf[key_idx] & bitset_mask) >> (BITSET_NUM * u_bitset_idx);

        atomicAdd(b_count_buf[to_partition_radixes_offsets_idx(rad, THREAD_BLOCK_IDX)], 1);
        atomicAdd(b_tot_count_buf[rad], 1);
    }
}
