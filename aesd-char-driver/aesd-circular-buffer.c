/**
 * @file aesd-circular-buffer.c
 * @brief Functions and data related to a circular buffer imlementation
 *
 * @author Dan Walkes
 * @date 2020-03-01
 * @copyright Copyright (c) 2020
 *
 */

#ifdef __KERNEL__
#include <linux/string.h>
#else
#include <string.h>
#endif
#include "aesd-circular-buffer.h"

/**
 * @param buffer the buffer to search for corresponding offset.  Any necessary locking must be performed by caller.
 * @param char_offset the position to search for in the buffer list, describing the zero referenced
 *      character index if all buffer strings were concatenated end to end
 * @param entry_offset_byte_rtn is a pointer specifying a location to store the byte of the returned aesd_buffer_entry
 *      buffptr member corresponding to char_offset.  This value is only set when a matching char_offset is found
 *      in aesd_buffer.
 * @return the struct aesd_buffer_entry structure representing the position described by char_offset, or
 * NULL if this position is not available in the buffer (not enough data is written).
 */
struct aesd_buffer_entry *aesd_circular_buffer_find_entry_offset_for_fpos(struct aesd_circular_buffer *buffer,
            size_t char_offset, size_t *entry_offset_byte_rtn )
{
    size_t offset = buffer->out_offs;
    size_t buffer_size = sizeof(buffer->entry) / sizeof(buffer->entry[0]);
    size_t posit = 0;
    
    if (buffer->full) 
    {
      	if (char_offset < posit + buffer->entry[offset].size) 
      	{
      		*entry_offset_byte_rtn = char_offset - posit;
        	return &buffer->entry[offset];
        }
        posit += buffer->entry[offset].size;
        offset = (offset + 1) % buffer_size;
    }
    
    while (offset != buffer->in_offs) 
    {
    	if (char_offset < posit + buffer->entry[offset].size) 
    	{
        	*entry_offset_byte_rtn = char_offset - posit;
        	return &buffer->entry[offset];
        }
        posit += buffer->entry[offset].size;
        offset = (offset + 1) % buffer_size;
    }
    return NULL;
}

size_t aesd_circular_buffer_find_position(struct aesd_circular_buffer *buffer,
            size_t command_offset, size_t internal_offset)
{
    size_t offset = buffer->out_offs;
    size_t buffer_size = sizeof(buffer->entry) / sizeof(buffer->entry[0]);
    size_t entry = 0;
    size_t end_posit = 0; 

    if (buffer->full) {
        if (command_offset != entry) 
        {
            end_posit += buffer->entry[offset].size;
            offset = (offset + 1) % buffer_size;
            entry++;
        } 
        else 
        {
            if (buffer->entry[offset].size >= internal_offset)
            {
                end_posit += internal_offset;
            }	
            return end_posit;
        }
    }

    while (offset != buffer->in_offs) {
        if (command_offset != entry) 
        {
            end_posit += buffer->entry[offset].size;
            offset = (offset + 1) % buffer_size;
            entry++;
        } 
        else 
        {
            if (buffer->entry[offset].size >= internal_offset)
            {
                end_posit += internal_offset;
            }
	return end_posit;
        }
    }

    return end_posit;
}

/**
* Adds entry @param add_entry to @param buffer in the location specified in buffer->in_offs.
* If the buffer was already full, overwrites the oldest entry and advances buffer->out_offs to the
* new start location.
* Any necessary locking must be handled by the caller
* Any memory referenced in @param add_entry must be allocated by and/or must have a lifetime managed by the caller.
*/

const char *aesd_circular_buffer_add_entry(struct aesd_circular_buffer *buffer, const struct aesd_buffer_entry *add_entry)
{
    	buffer->entry[buffer->in_offs] = *add_entry;
    	buffer->in_offs = (buffer->in_offs + 1) % AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED;
    	
    	if(buffer->full)                                                                   
    	{
        	buffer->out_offs = buffer->in_offs;
    	}
    	
    	if(buffer->in_offs == buffer->out_offs)                                           
   	{
        	buffer->full = true;
    	}
    	else
    	{
        	buffer->full = false;
    	}
    	return NULL;
}


/**
* Initializes the circular buffer described by @param buffer to an empty struct
*/
void aesd_circular_buffer_init(struct aesd_circular_buffer *buffer)
{
    memset(buffer,0,sizeof(struct aesd_circular_buffer));
}
