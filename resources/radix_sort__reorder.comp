#version 460

#define THREAD_IDX        gl_LocalInvocationIndex
#define THREADS_NUM       64
#define THREAD_BLOCK_IDX  (gl_WorkGroupID.x + gl_NumWorkGroups.x * (gl_WorkGroupID.y + gl_NumWorkGroups.z * gl_WorkGroupID.z))
#define THREAD_BLOCKS_NUM (gl_NumWorkGroups.x * gl_NumWorkGroups.y * gl_NumWorkGroups.z)
#define ITEMS_NUM         4
#define BITSET_NUM        4
#define BITSET_SIZE       uint(exp2(BITSET_NUM))

layout(local_size_x = THREADS_NUM, local_size_y = 1, local_size_z = 1) in;

layout(std430, binding = 0) restrict readonly buffer in_buffer
{
    uint b_in_values[];
};

layout(std430, binding = 1) restrict writeonly buffer out_buffer
{
    uint b_out_values[];
};

layout(std430, binding = 2) buffer ssbo_local_off_buf  { uint b_local_offsets_buf[]; };
layout(std430, binding = 3) buffer ssbo_glob_count_buf { uint b_glob_counts_buf[BITSET_SIZE]; }; // todo

uniform uint u_arr_len;
uniform uint u_bitset_idx;

#define UINT32_MAX uint(-1)

shared uint s_prefix_sum[BITSET_SIZE][THREADS_NUM * ITEMS_NUM];
shared uint s_key_buf[THREADS_NUM * ITEMS_NUM][2];
shared uint s_count[BITSET_SIZE];

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
    // ------------------------------------------------------------------------------------------------
    // Global offsets
    // ------------------------------------------------------------------------------------------------

    // Runs an exclusive scan on the global counts to get the starting offset for each radix.
    // The offsets are global for the whole input array.

    uint glob_off_buf[BITSET_SIZE];

    // Exclusive scan
    for (uint sum = 0, i = 0; i < BITSET_SIZE; i++)
    {
        glob_off_buf[i] = sum;
        sum += b_glob_counts_buf[i];
    }

    // ------------------------------------------------------------------------------------------------
    // Radix sort on partition
    // ------------------------------------------------------------------------------------------------

    // NOTE:
    // The last partition potentially can have keys that aren't part of the original array.
    // These keys during the radix sort are set to `UINT32_MAX` (or 0xFFFFFFFF), this way, after the array is sorted
    // they'll always be in the last position and won't be confused with the original keys of the input array.

    for (uint item_idx = 0; item_idx < ITEMS_NUM; item_idx++)
    {
        uint key_idx = to_key_idx(item_idx, THREAD_IDX, THREAD_BLOCK_IDX);
        uint loc_idx = to_loc_idx(item_idx, THREAD_IDX);
        s_key_buf[loc_idx][0] = key_idx < u_arr_len ? b_in_values[key_idx] : UINT32_MAX; // If the key_idx is outside of the input array then use UINT32_MAX.
        s_key_buf[loc_idx][1] = UINT32_MAX;
    }

    barrier();

    // The offsets of the radixes within the partition. The values of this array will be written every radix-sort iteration.
    uint in_partition_group_off[BITSET_SIZE];

    uint bitset_idx;
    for (bitset_idx = 0; bitset_idx <= u_bitset_idx; bitset_idx++)
    {
        uint bitset_mask = (BITSET_SIZE - 1) << (BITSET_NUM * bitset_idx);

        // Init
        for (uint item_idx = 0; item_idx < ITEMS_NUM; item_idx++)
        {
            for (uint bitset_val = 0; bitset_val < BITSET_SIZE; bitset_val++)
            {
                uint loc_idx = to_loc_idx(item_idx, THREAD_IDX);
                s_prefix_sum[bitset_val][loc_idx] = 0;
            }
        }

        barrier();

        // ------------------------------------------------------------------------------------------------
        // Predicate test
        // ------------------------------------------------------------------------------------------------

        // For each key of the current partition sets a 1 whether the radix corresponds to the radix of the current key.
        // Otherwise the default value is initialized to 0.

        for (uint item_idx = 0; item_idx < ITEMS_NUM; item_idx++)
        {
            uint loc_idx = to_loc_idx(item_idx, THREAD_IDX);
            uint k = s_key_buf[loc_idx][bitset_idx % 2];
            uint radix = (k & bitset_mask) >> (BITSET_NUM * bitset_idx);
            s_prefix_sum[radix][loc_idx] = 1;
        }

        barrier();

        // ------------------------------------------------------------------------------------------------
        // Exclusive sum
        // ------------------------------------------------------------------------------------------------

        // THREADS_NUM * ITEMS_NUM are guaranteed to be a power of two, otherwise it won't work!

        // An exclusive sum is run on the predicates, this way, for each location in the partition
        // we know the offset the element has to go, relative to its radix.

        // Up-sweep
        for (uint d = 0; d < uint(log2(THREADS_NUM * ITEMS_NUM)); d++)
        {
            for (uint item_idx = 0; item_idx < ITEMS_NUM; item_idx++)
            {
                uint step = uint(exp2(d));
                uint loc_idx = to_loc_idx(item_idx, THREAD_IDX);

                if (loc_idx % (step * 2) == 0)
                {
                    uint from_idx = loc_idx + (step - 1);
                    uint to_idx = from_idx + step;

                    if (to_idx < THREADS_NUM * ITEMS_NUM)
                    {
                        for (uint bitset_val = 0; bitset_val < BITSET_SIZE; bitset_val++)
                        {
                            s_prefix_sum[bitset_val][to_idx] = s_prefix_sum[bitset_val][from_idx] + s_prefix_sum[bitset_val][to_idx];
                        }
                    }
                }
            }

            barrier();
        }

        // Clear last
        if (THREAD_IDX == 0)
        {
            for (uint bitset_val = 0; bitset_val < BITSET_SIZE; bitset_val++)
            {
                s_prefix_sum[bitset_val][(THREADS_NUM * ITEMS_NUM) - 1] = 0;
            }
        }

        barrier();

        // Down-sweep
        for (int d = int(log2(THREADS_NUM * ITEMS_NUM)) - 1; d >= 0; d--)
        {
            for (uint item_idx = 0; item_idx < ITEMS_NUM; item_idx++)
            {
                uint step = uint(exp2(d));
                uint loc_idx = to_loc_idx(item_idx, THREAD_IDX);

                if (loc_idx % (step * 2) == 0)
                {
                    uint from_idx = loc_idx + (step - 1);
                    uint to_idx = from_idx + step;

                    if (to_idx < THREADS_NUM * ITEMS_NUM)
                    {
                        for (uint bitset_val = 0; bitset_val < BITSET_SIZE; bitset_val++)
                        {
                            uint r = s_prefix_sum[bitset_val][to_idx];
                            s_prefix_sum[bitset_val][to_idx] = r + s_prefix_sum[bitset_val][from_idx];
                            s_prefix_sum[bitset_val][from_idx] = r;
                        }
                    }
                }
            }

            barrier();
        }

        // ------------------------------------------------------------------------------------------------
        // Shuffling
        // ------------------------------------------------------------------------------------------------

        uint last_loc_idx;
        if (THREAD_BLOCK_IDX == (THREAD_BLOCKS_NUM - 1)) { // The last partition may be larger than the original size of the array, so the last position is before its end.
            last_loc_idx = u_arr_len - (THREAD_BLOCKS_NUM - 1) * (THREADS_NUM * ITEMS_NUM) - 1;
        } else {
            last_loc_idx = (THREADS_NUM * ITEMS_NUM) - 1;
        }

        // Now for every radix we need to know its "global" offset within the partition (we only know relative offsets per location within the same radix).
        // So we just issue an exclusive scan (again) on the last element offset of every radix.

        for (uint sum = 0, i = 0; i < BITSET_SIZE; i++) // Small prefix-sum to calculate group offsets.
        {
            in_partition_group_off[i] = sum;

            // Since we need the count of every bit-group, if the last element is the bitset value itself, we need to count it.
            // Otherwise it will be already counted.
            bool is_last = ((s_key_buf[last_loc_idx][bitset_idx % 2] & bitset_mask) >> (BITSET_NUM * bitset_idx)) == i;
            sum += s_prefix_sum[i][last_loc_idx] + (is_last ? 1 : 0);
        }

        for (uint item_idx = 0; item_idx < ITEMS_NUM; item_idx++)
        {
            uint loc_idx = to_loc_idx(item_idx, THREAD_IDX);
            uint k = s_key_buf[loc_idx][bitset_idx % 2];
            uint radix = (k & bitset_mask) >> (BITSET_NUM * bitset_idx);

            // The destination address is calculated as the global offset of the radix plus the
            // local offset that depends by the location of the element.
            uint dest_addr = in_partition_group_off[radix] + s_prefix_sum[radix][loc_idx];
            s_key_buf[dest_addr][(bitset_idx + 1) % 2] = k; // Double-buffering is used every cycle the read and write buffer are swapped.
        }

        barrier();
    }

    // TODO (REMOVE) WRITE s_prefix_sum
    /*
    for (uint item_idx = 0; item_idx < ITEMS_NUM; item_idx++)
    {
        uint key_idx = to_key_idx(item_idx, THREAD_IDX, THREAD_BLOCK_IDX);
        if (item_idx < u_arr_len)
        {
            uint loc_idx = to_loc_idx(item_idx, THREAD_IDX);
            b_out_values[key_idx] = s_prefix_sum[1][loc_idx];
        }
    }

    for (uint item_idx = 0; item_idx < ITEMS_NUM; item_idx++)
    {
        uint key_idx = to_key_idx(item_idx, THREAD_IDX, THREAD_BLOCK_IDX);
        if (key_idx < u_arr_len)
        {
            uint loc_idx = to_loc_idx(item_idx, THREAD_IDX);
            b_out_values[key_idx] = s_key_buf[loc_idx][bitset_idx % 2];
        }
    }
    */

    // ------------------------------------------------------------------------------------------------
    // Scattered writes to sorted partitions
    // ------------------------------------------------------------------------------------------------

    // TODO DUPLICATION ISSUE IS INVOLVED HERE!

    uint bitset_mask = (BITSET_SIZE - 1) << (BITSET_NUM * u_bitset_idx);

    /*
    if (THREAD_IDX == 0)
    {
        for (uint i = 0; i < BITSET_SIZE; i++) {
            s_count[i] = 0;
        }
    }

    barrier();

    for (uint item_idx = 0; item_idx < ITEMS_NUM; item_idx++)
    {
        uint key_idx = to_key_idx(item_idx, THREAD_IDX, THREAD_BLOCK_IDX);
        if (key_idx < u_arr_len)
        {
            uint k = s_key_buf[to_loc_idx(item_idx, THREAD_IDX)][bitset_idx % 2];
            uint rad = (k & bitset_mask) >> (BITSET_NUM * u_bitset_idx);

            atomicAdd(s_count[rad], 1);
        }
    }

    barrier();

    for (uint item_idx = 0; item_idx < ITEMS_NUM; item_idx++)
    {
        uint key_idx = to_key_idx(item_idx, THREAD_IDX, THREAD_BLOCK_IDX);
        if (key_idx < u_arr_len)
        {
            uint k = s_key_buf[to_loc_idx(item_idx, THREAD_IDX)][bitset_idx % 2];
            uint rad = (k & bitset_mask) >> (BITSET_NUM * u_bitset_idx);

            b_out_values[key_idx] = s_count[rad];
        }
    }

    barrier();
    */

    for (uint item_idx = 0; item_idx < ITEMS_NUM; item_idx++)
    {
        uint key_idx = to_key_idx(item_idx, THREAD_IDX, THREAD_BLOCK_IDX);
        if (key_idx < u_arr_len)
        {
            uint loc_idx = to_loc_idx(item_idx, THREAD_IDX);
            uint k = s_key_buf[loc_idx][bitset_idx % 2];
            uint rad = (k & bitset_mask) >> (BITSET_NUM * u_bitset_idx);

            uint glob_off = glob_off_buf[rad];
            uint local_off = b_local_offsets_buf[to_partition_radixes_offsets_idx(rad, THREAD_BLOCK_IDX)];

            uint dest_idx = glob_off + local_off + (loc_idx - in_partition_group_off[rad]);
            b_out_values[dest_idx] = k;
        }
    }
}
