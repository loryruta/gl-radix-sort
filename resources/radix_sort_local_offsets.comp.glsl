
#define THREAD_IDX        gl_LocalInvocationIndex
#define THREADS_NUM       64
#define THREAD_BLOCK_IDX  (gl_WorkGroupID.x + gl_NumWorkGroups.x * (gl_WorkGroupID.y + gl_NumWorkGroups.z * gl_WorkGroupID.z))
#define ITEMS_NUM         4u
#define BITSET_NUM        4u
#define BITSET_SIZE       16u

#define OP_UPSWEEP    0u
#define OP_CLEAR_LAST 1u
#define OP_DOWNSWEEP  2u

layout(local_size_x = THREADS_NUM, local_size_y = 1, local_size_z = 1) in;

layout(std430, binding = 0) buffer ssbo_local_offsets_buf { uint b_local_offsets_buf[]; }; // b_count_buf[THREAD_BLOCKS_NUM * BITSET_SIZE]

uniform uint u_arr_len; // Already guaranteed to be a power of 2
uniform uint u_depth;
uniform uint u_op;

uint to_partition_radixes_offsets_idx(uint radix, uint thread_block_idx)
{
    return radix * u_arr_len + thread_block_idx;
}

uint to_loc_idx(uint item_idx, uint thread_idx)
{
    return (thread_idx * ITEMS_NUM + item_idx);
}

uint to_key_idx(uint item_idx, uint thread_idx, uint thread_block_idx)
{
    return (thread_block_idx * ITEMS_NUM * uint(THREADS_NUM)) + (thread_idx * ITEMS_NUM) + item_idx;
}

void main()
{
    if (uint(fract(log2(float(u_arr_len)))) != 0u) {
        return; // ERROR: The u_arr_len must be a power of 2 otherwise the Blelloch scan won't work!
    }

    // ------------------------------------------------------------------------------------------------
    // Blelloch scan
    // ------------------------------------------------------------------------------------------------

    uint step = uint(exp2(float(u_depth)));

    if (u_op == OP_UPSWEEP)
    {
        // Reduce (upsweep)
        for (uint item_idx = 0u; item_idx < ITEMS_NUM; item_idx++)
        {
            uint key_idx = to_key_idx(item_idx, THREAD_IDX, THREAD_BLOCK_IDX);
            if (key_idx % (step * 2u) == 0u)
            {
                uint from_idx = key_idx + (step - 1u);
                uint to_idx = from_idx + step;

                if (to_idx < u_arr_len)
                {
                    for (uint rad = 0u; rad < BITSET_SIZE; rad++)
                    {
                        uint from_rad_idx = to_partition_radixes_offsets_idx(rad, from_idx);
                        uint to_rad_idx = to_partition_radixes_offsets_idx(rad, to_idx);

                        b_local_offsets_buf[to_rad_idx] = b_local_offsets_buf[from_rad_idx] + b_local_offsets_buf[to_rad_idx];
                    }
                }
            }
        }
    }
    else if (u_op == OP_DOWNSWEEP)
    {
        // Downsweep
        for (uint item_idx = 0u; item_idx < ITEMS_NUM; item_idx++)
        {
            uint key_idx = to_key_idx(item_idx, THREAD_IDX, THREAD_BLOCK_IDX);
            if (key_idx % (step * 2u) == 0u)
            {
                uint from_idx = key_idx + (step - 1u);
                uint to_idx = from_idx + step;

                if (to_idx < u_arr_len)
                {
                    for (uint rad = 0u; rad < BITSET_SIZE; rad++)
                    {
                        uint from_rad_idx = to_partition_radixes_offsets_idx(rad, from_idx);
                        uint to_rad_idx = to_partition_radixes_offsets_idx(rad, to_idx);

                        uint r = b_local_offsets_buf[to_rad_idx];
                        b_local_offsets_buf[to_rad_idx] = b_local_offsets_buf[from_rad_idx] + b_local_offsets_buf[to_rad_idx];
                        b_local_offsets_buf[from_rad_idx] = r;
                    }
                }
            }
        }
    }
    else// if (u_op == OP_CLEAR_LAST)
    {
        for (uint item_idx = 0u; item_idx < ITEMS_NUM; item_idx++)
        {
            uint key_idx = to_key_idx(item_idx, THREAD_IDX, THREAD_BLOCK_IDX);
            if (key_idx == (u_arr_len - 1u))
            {
                for (uint rad = 0u; rad < BITSET_SIZE; rad++)
                {
                    uint idx = to_partition_radixes_offsets_idx(rad, key_idx);
                    b_local_offsets_buf[idx] = 0u;
                }
            }
        }
    }
}
