/* -*- P4_16 -*- */
#include <core.p4>
#include <v1model.p4>

const bit<16> TYPE_IPV4 = 0x800;
const bit<8> IPPROTO_UDP = 0x11;
const bit<8> ZERO=0;

/*************************************************************************
*********************** H E A D E R S  ***********************************
*************************************************************************/

typedef bit<16>  egressSpec_t;
typedef bit<48> macAddr_t;
typedef bit<32> ip4Addr_t;

typedef bit<16> bit16_t;
typedef bit<48> bit48_t;
typedef bit<32> bit32_t;
typedef int<32> int32_t;
typedef int<16> int16_t;

header ethernet_t {
    macAddr_t dstAddr;
    macAddr_t srcAddr;
    bit<16>   etherType;
}

header ipv4_t {
    bit<4>    version;
    bit<4>    ihl;
    bit<8>    diffserv;
    bit<16>   totalLen;
    bit<16>   identification;
    bit<3>    flags;
    bit<13>   fragOffset;
    bit<8>    ttl;
    bit<8>    protocol;
    bit<16>   hdrChecksum;
    ip4Addr_t srcAddr;
    ip4Addr_t dstAddr;
}

header udp_t {
    bit16_t srcPort;
    bit16_t dstPort;
    bit16_t length;
    bit16_t checksum;
}

/*custom defined header for coordinates - 98 bytes */
header coordinate_metadata_t {
    bit16_t sid;
    bit16_t robot_id;
    int16_t x;
    int16_t y;
    int16_t z;
    int16_t qx;
    int16_t qy;
    int16_t qz;
    int16_t qw;
    bit16_t b1;
    bit16_t b2;
    bit16_t f0;
    bit16_t f1;
    bit16_t f2;
    bit16_t f3;
    bit16_t f4;
    bit16_t f5;
    bit16_t f6;
    bit16_t f7;
    bit16_t f8;
    bit16_t f9;
    bit16_t f10;
    bit16_t f11;
    bit16_t f12;
    bit16_t f13;
    bit16_t f14;
    bit16_t key0;
    bit16_t key1;
}

/* custom defined header for telemetry - 22 bytes */
header telemetry_t {
    bit<64> ing_glb_tstamp;
    bit<64> cur_glb_tstamp;
    bit16_t pkt_len;
    bit16_t ig_port;
    bit16_t eg_port;
    bit16_t dummy;
}


// header append_t {
//     bit16_t app_val;
// }

header intrinsic_metadata_t {
    bit<64> ingress_global_timestamp;
    bit<64> current_global_timestamp;
}

struct metadata {
    intrinsic_metadata_t intrinsic_metadata;
}

struct headers {
    ethernet_t ethernet;
    ipv4_t ipv4;
    udp_t udp;
    coordinate_metadata_t coordinate_metadata;
    telemetry_t telemetry;
    //append_t append;
    
}


register<int<16>>(3) old_x_y_z;
//register<int<16>>(3) sign_x_y_z;
//register<int<16>>(6) diff_raw_abs;
//register<bit<1>>(5) comparisons;
register<int<16>>(2) Count;


extern void denoise();


/*************************************************************************
*********************** P A R S E R  ***********************************
*************************************************************************/

parser MyParser(packet_in packet,
                out headers hdr,
                inout metadata meta,
                inout standard_metadata_t standard_metadata) {

    state start {
        transition parse_ethernet;
    }

    state parse_ethernet {
        packet.extract(hdr.ethernet);
        transition select(hdr.ethernet.etherType) {
            TYPE_IPV4: parse_ipv4;
            default: accept;
        }
    }
    state parse_ipv4 {
        packet.extract(hdr.ipv4);
        transition select(hdr.ipv4.protocol) {
            IPPROTO_UDP: parse_udp;
            default: accept;
        }
    }
    state parse_udp {
        packet.extract(hdr.udp);
        transition parse_coordinate_metadata;
    }

    state parse_coordinate_metadata {
        packet.extract(hdr.coordinate_metadata);
        transition parse_telemetry;
    }   

    state parse_telemetry {
        packet.extract(hdr.telemetry);
        transition accept;
    }

}

/*************************************************************************
************   C H E C K S U M    V E R I F I C A T I O N   *************
*************************************************************************/

control MyVerifyChecksum(inout headers hdr, inout metadata meta) {
    apply {  }
}


/*************************************************************************
**************  I N G R E S S   P R O C E S S I N G   *******************
*************************************************************************/

control MyIngress(inout headers hdr,
                  inout metadata meta,
                  inout standard_metadata_t standard_metadata) {
    
    int <16> temp_x;
    int <16> temp_y;
    int <16> temp_z;
    int <16> diff_x;
    int <16> diff_y;
    int <16> diff_z;

    action drop() {
        mark_to_drop();
    }

    action mac_forward(egressSpec_t port){
        standard_metadata.egress_spec = port;
    }

    table mac_exact {
        key = {
            standard_metadata.ingress_port: exact;
        }
        actions = {
            mac_forward;
            drop;
            NoAction;
        }
        size = 1024;
        default_action = NoAction;
    }
    
    apply {      
        
        if (hdr.coordinate_metadata.isValid()) 
        {
            if (standard_metadata.ingress_port==0)
            { 
                hdr.ipv4.ttl = 55;
                //incoming on p0
                //mac_exact.apply(); //just forward from P0 to P1 (P0 is connected to the outside network, P1 to the Geomagic Touch)
            }
                
            else if (standard_metadata.ingress_port==1) 
            { 
                hdr.ipv4.ttl = 66;
                denoise();
                //mac_exact.apply();
            }

        }
        else
        {
            hdr.ipv4.ttl = 77;
            //mac_exact.apply(); //for all packets other than this specific custom type
        }

        mac_exact.apply();
        

    }
}

/*************************************************************************
****************  E G R E S S   P R O C E S S I N G   *******************
*************************************************************************/

control MyEgress(inout headers hdr,
                 inout metadata meta,
                 inout standard_metadata_t standard_metadata) {
    apply { 
        hdr.telemetry.ing_glb_tstamp = meta.intrinsic_metadata.ingress_global_timestamp;
        hdr.telemetry.cur_glb_tstamp = meta.intrinsic_metadata.current_global_timestamp;
     }
}

/*************************************************************************
*************   C H E C K S U M    C O M P U T A T I O N   **************
*************************************************************************/

control MyComputeChecksum(inout headers  hdr, inout metadata meta) {
    bit<16> old_udp_checksum;
    apply {
	update_checksum(
	    hdr.ipv4.isValid(),{ 
            hdr.ipv4.version,
            hdr.ipv4.ihl,
            hdr.ipv4.diffserv,
            hdr.ipv4.totalLen,
            hdr.ipv4.identification,
            hdr.ipv4.flags,
            hdr.ipv4.fragOffset,
            hdr.ipv4.ttl,
            hdr.ipv4.protocol,
            hdr.ipv4.srcAddr,
            hdr.ipv4.dstAddr 
            },
            hdr.ipv4.hdrChecksum,
            HashAlgorithm.csum16);

        update_checksum_with_payload(
        hdr.udp.isValid(),{
            hdr.ipv4.srcAddr,
            hdr.ipv4.dstAddr,
            ZERO,
            hdr.ipv4.protocol,
            hdr.ipv4.totalLen,
            hdr.udp.srcPort,
            hdr.udp.dstPort,
            hdr.udp.length,
            old_udp_checksum,
            hdr.coordinate_metadata.sid,
            hdr.coordinate_metadata.robot_id,
            hdr.coordinate_metadata.x,
            hdr.coordinate_metadata.y,
            hdr.coordinate_metadata.z,
            hdr.coordinate_metadata.qx,
            hdr.coordinate_metadata.qy,
            hdr.coordinate_metadata.qz,
            hdr.coordinate_metadata.qw,
            hdr.coordinate_metadata.b1,
            hdr.coordinate_metadata.b2,
            hdr.coordinate_metadata.f0,
            hdr.coordinate_metadata.f1,
            hdr.coordinate_metadata.f2,
            hdr.coordinate_metadata.f3,
            hdr.coordinate_metadata.f4,
            hdr.coordinate_metadata.f5,
            hdr.coordinate_metadata.f6,
            hdr.coordinate_metadata.f7,
            hdr.coordinate_metadata.f8,
            hdr.coordinate_metadata.f9,
            hdr.coordinate_metadata.f10,
            hdr.coordinate_metadata.f11,
            hdr.coordinate_metadata.f12,
            hdr.coordinate_metadata.f13,
            hdr.coordinate_metadata.f14,
            hdr.coordinate_metadata.key0,
            hdr.coordinate_metadata.key1,
            hdr.telemetry.ing_glb_tstamp,
            hdr.telemetry.cur_glb_tstamp,
            hdr.telemetry.pkt_len,
            hdr.telemetry.ig_port,
            hdr.telemetry.eg_port,
            hdr.telemetry.dummy
            },
        hdr.udp.checksum, 
        HashAlgorithm.csum16);
    }
}

/*************************************************************************
***********************  D E P A R S E R  *******************************
*************************************************************************/

control MyDeparser(packet_out packet, in headers hdr) {
    apply {
        packet.emit(hdr.ethernet);
        packet.emit(hdr.ipv4);
        packet.emit(hdr.udp);
        
        packet.emit(hdr.coordinate_metadata);
        packet.emit(hdr.telemetry);
        //packet.emit(hdr.append);
        
    }
}

/*************************************************************************
***********************  S W I T C H  *******************************
*************************************************************************/

V1Switch(
MyParser(),
MyVerifyChecksum(),
MyIngress(),
MyEgress(),
MyComputeChecksum(),
MyDeparser()
) main;
