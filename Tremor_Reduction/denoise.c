#include <stdint.h>
#include <nfp/me.h>
#include <nfp/mem_atomic.h>
#include <pif_common.h>
#include "pif_plugin.h"
#include "pif_registers.h"
#include "out/pif_headers.h"

int pif_plugin_denoise(EXTRACTED_HEADERS_T *headers, MATCH_DATA_T *match_data){


    
    uint16_t new_x, new_y, new_z; 
    uint16_t old_x, old_y, old_z; 
    uint16_t threshold = 5;
    uint16_t diff_x, diff_y, diff_z; 

    

    PIF_PLUGIN_coordinate_metadata_T *coordinate_metadata;
    coordinate_metadata = pif_plugin_hdr_get_coordinate_metadata(headers);

    old_x = pif_register_old_x_y_z[(0)].value;
    old_y = pif_register_old_x_y_z[(1)].value;
    old_z = pif_register_old_x_y_z[(2)].value;
    new_x = coordinate_metadata->x;
    new_y = coordinate_metadata->y;
    new_z = coordinate_metadata->z;

    diff_x = new_x - old_x; 
    diff_y = new_y - old_y; 
    diff_z = new_z - old_z; 

    if ((diff_x >> 15) != 0)
    {
        diff_x = ~diff_x +1 ;
    }
    else
    {
        diff_x =  diff_x ;
    }

    if ((diff_y >> 15) != 0)
    {
        diff_y = ~diff_y +1 ;
    }
    else
    {
        diff_y =  diff_y ;
    }


    if ((diff_z >> 15) != 0)
    {
        diff_z = ~diff_z +1 ;
    }
    else
    {
        diff_z =  diff_z ;
    }


    if ( (diff_x > threshold) || (diff_y > threshold) || (diff_z > threshold) )
    {
        pif_register_old_x_y_z[(0)].value = new_x;
        pif_register_old_x_y_z[(1)].value = new_y;
        pif_register_old_x_y_z[(2)].value = new_z;
        pif_register_Count[(0)].value = pif_register_Count[(0)].value + 1;
        return PIF_PLUGIN_RETURN_FORWARD;
        
    }
    else
    {
        pif_register_Count[(1)].value = pif_register_Count[(1)].value + 1;
        return PIF_PLUGIN_RETURN_DROP;
    }
    
    
}