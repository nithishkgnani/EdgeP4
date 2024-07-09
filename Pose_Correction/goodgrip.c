#include <stdint.h>
#include <nfp/me.h>
#include <nfp/mem_atomic.h>
#include <pif_common.h>
#include "pif_plugin.h"
#include "pif_registers.h"
#include "out/pif_headers.h"
#define  fsr(i) pif_register_fsr[(i)].value
int pif_plugin_goodgrip(EXTRACTED_HEADERS_T *headers, MATCH_DATA_T *match_data){
    int16_t i=9;
    int16_t j=13;
    int16_t threshold = 500;
    PIF_PLUGIN_coordinate_metadata_T *coordinate_metadata;
    coordinate_metadata = pif_plugin_hdr_get_coordinate_metadata(headers);
    fsr(0) = coordinate_metadata->f0;
    fsr(1) = coordinate_metadata->f1;
    fsr(2) = coordinate_metadata->f2;
    fsr(3) = coordinate_metadata->f3;
    fsr(4) = coordinate_metadata->f4;
    fsr(5) = coordinate_metadata->f5;
    fsr(6) = coordinate_metadata->f6;
    fsr(7) = coordinate_metadata->f7;
    fsr(8) = coordinate_metadata->f8;
    fsr(9) = coordinate_metadata->f9;
    fsr(10) = coordinate_metadata->f10;
    fsr(11) = coordinate_metadata->f11;
    fsr(12) = coordinate_metadata->f12;
    fsr(13) = coordinate_metadata->f13;
    fsr(14) = coordinate_metadata->f14;
    // Find the index that breaches threshold value going from start to end
    for(i=0; i<10; i++){
        if(fsr(i) > threshold){
            pif_register_se[0].value = i;
            // coordinate_metadata->key0=i;
            break;
        }
    }
    // Find the index that breaches threshold value going from end to start
    for(j=10; j>0; j--){
        if(fsr(j-1) > threshold){
            pif_register_es[0].value = j-1;
            // coordinate_metadata->key1=j-1;
            break;
        }
    }
    return 0;
}