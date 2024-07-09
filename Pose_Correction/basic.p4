 #include <core.p4>
#include <v1model.p4>

#define PKT_INSTANCE_TYPE_INGRESS_CLONE 1

const bit<16> TYPE_MetaData  = 0x2700;
const bit<16> ETHERTYPE_IPV4 = 0x800;
const bit<8> IPPROTO_UDP = 17;
const bit<8> ZERO=0;

/*************************************************************************
*********************** H E A D E R S  ***********************************
*************************************************************************/

typedef bit<16> bit16_t;
typedef bit<48> bit48_t;
typedef bit<32> bit32_t;
typedef int<32> int32_t;
typedef int<16> int16_t;


/* custom defined header for telemetry - 24 bytes */
header telemetry_t {
    bit<64> ing_glb_tstamp;
    bit<64> cur_glb_tstamp;
    bit16_t pkt_len;
    bit16_t ig_port;
    bit16_t eg_port;
    bit16_t dummy;
}

header ethernet_t {
    bit48_t dstAddr;
    bit48_t srcAddr;
    bit16_t   etherType;
}

header ipv4_t {
    bit<4>    version;
    bit<4>    ihl;
    bit<8>    diffserv;
    bit16_t   totalLen;
    bit16_t   identification;
    bit<3>    flags;
    bit<13>   fragOffset;
    bit<8>    ttl;
    bit<8>    protocol;
    bit16_t   hdrChecksum;
    bit32_t srcAddr;
    bit32_t dstAddr;
}

header udp_t {
    bit16_t srcPort;
    bit16_t dstPort;
    bit16_t length;
    bit16_t checksum;
}

/*custom defined header for coordinates - 56 bytes */
header coordinate_metadata_t {
    int16_t sid;
    int16_t task_id;
    int16_t x;
    int16_t y;
    int16_t z;
    int16_t qx;
    int16_t qy;
    int16_t qz;
    int16_t qw;
    int16_t b1;
    int16_t b2;
    int16_t f0;
    int16_t f1;
    int16_t f2;
    int16_t f3;
    int16_t f4;
    int16_t f5;
    int16_t f6;
    int16_t f7;
    int16_t f8;
    int16_t f9;
    int16_t f10;
    int16_t f11;
    int16_t f12;
    int16_t f13;
    int16_t f14;
    bit16_t key0;
    bit16_t key1;
}

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
}

// Every register is an array by default. Hence have to mention that it is one element.
register<bit<16>>(1) se;
register<bit<16>>(1) es;
register<bit<16>>(15) fsr;
register<bit<16>>(1) fake_ipv4_id;
extern void goodgrip();

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
            ETHERTYPE_IPV4: parse_ipv4;
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

    bit<16> startEnd;
    bit<16> endStart;

    action drop() {
        mark_to_drop();
    }

    action port_forward(bit16_t port) {
        standard_metadata.egress_spec = port;
        hdr.telemetry.ing_glb_tstamp = meta.intrinsic_metadata.ingress_global_timestamp;       
    }
    action port_forward_spoof(bit16_t port){
        hdr.telemetry.ing_glb_tstamp = meta.intrinsic_metadata.ingress_global_timestamp;
        standard_metadata.egress_spec = port;
        // fake_ipv4_id.read(hdr.ipv4.identification,0);
        // fake_ipv4_id.write(0,hdr.ipv4.identification+1);
    }
    action debug(){
        standard_metadata.egress_spec=768;
    }
    action update_coordinate(bit16_t port, int16_t h_step, int16_t v_step, int16_t quatx, int16_t quaty, int16_t quatz, int16_t quatw) {
        hdr.telemetry.ing_glb_tstamp = meta.intrinsic_metadata.ingress_global_timestamp;
        standard_metadata.egress_spec = port;
        hdr.coordinate_metadata.x = hdr.coordinate_metadata.x + 8*(h_step);
        hdr.coordinate_metadata.z = hdr.coordinate_metadata.x + 8*(v_step);
        hdr.coordinate_metadata.qx = quatx;
        hdr.coordinate_metadata.qy = quaty;
        hdr.coordinate_metadata.qz = quatz;
        hdr.coordinate_metadata.qw = quatw;
        //debug
        hdr.coordinate_metadata.task_id = hdr.coordinate_metadata.task_id+10;
        // swap MAC addresses for reflecting packet
        bit<48> temp=hdr.ethernet.dstAddr;
        hdr.ethernet.dstAddr=hdr.ethernet.srcAddr;
        hdr.ethernet.srcAddr=temp;

        // swap IP addresses
        bit<32> ip=hdr.ipv4.srcAddr;
        hdr.ipv4.srcAddr=hdr.ipv4.dstAddr;
        hdr.ipv4.dstAddr=ip;
        // swap UDP ports
        bit<16> newport=hdr.udp.dstPort;
        hdr.udp.dstPort=hdr.udp.srcPort;
        hdr.udp.srcPort=newport;
        // fake_ipv4_id.read(hdr.ipv4.identification,0);
        // fake_ipv4_id.write(0,hdr.ipv4.identification+1);
        // hdr.ipv4.ttl=10; 
    }

    action clone_packet() {
        const bit<32> REPORT_MIRROR_SESSION_ID = 500;
        // Clone from ingress to ingress pipeline
        clone(CloneType.I2I, REPORT_MIRROR_SESSION_ID);
    }

    table port_exact { 
        key = {
            standard_metadata.instance_type: exact;
            standard_metadata.ingress_port: exact;
        }
        actions = {
            port_forward;
            port_forward_spoof;
            drop;
            NoAction;
        }
        size = 1024;
        default_action = NoAction();
    }

    table update_coordinate_exact { 
        key = {
            standard_metadata.instance_type: exact;
            hdr.coordinate_metadata.key0: exact;
            hdr.coordinate_metadata.key1: exact;
        }
        actions = {
            update_coordinate;
            drop;
            debug;
            NoAction;
        }
        size = 1024;
        default_action = NoAction();
    }

    apply {
        if (hdr.udp.isValid()){
            if (hdr.ipv4.ttl==1){
                drop();
            }
            if (standard_metadata.ingress_port==0){
                goodgrip();
                se.read(hdr.coordinate_metadata.key0,0);  // Writing se register value to key0 field
                es.read(hdr.coordinate_metadata.key1,0);  // Writing es register value to key1 field
                if (hdr.coordinate_metadata.key0<hdr.coordinate_metadata.key1) clone_packet();  // clone if se<es
            }
            update_coordinate_exact.apply();
            port_exact.apply();
        }
        else{
            port_exact.apply();
        }
    }
}

/*************************************************************************
****************  E G R E S S   P R O C E S S I N G   *******************
*************************************************************************/

control MyEgress(inout headers hdr,
                 inout metadata meta,
                 inout standard_metadata_t standard_metadata) {

    apply {
        hdr.telemetry.pkt_len = (bit<16>)standard_metadata.packet_length;
        hdr.telemetry.ig_port = standard_metadata.ingress_port;
        hdr.telemetry.eg_port = standard_metadata.egress_port;
        hdr.telemetry.cur_glb_tstamp = meta.intrinsic_metadata.current_global_timestamp;
     }
}

/*************************************************************************
*************   C H E C K S U M    C O M P U T A T I O N   **************
*************************************************************************/
// update_checksum_with_payload<T, O>(in bool condition, in T data, inout O checksum, HashAlgorithm algo);
control MyComputeChecksum(inout headers  hdr, inout metadata meta) {
    apply {
        update_checksum(
        hdr.ipv4.isValid(),
            { hdr.ipv4.version,
              hdr.ipv4.ihl,
              hdr.ipv4.diffserv,
              hdr.ipv4.totalLen,
              hdr.ipv4.identification,
              hdr.ipv4.flags,
              hdr.ipv4.fragOffset,
              hdr.ipv4.ttl,
              hdr.ipv4.protocol,
              hdr.ipv4.srcAddr,
              hdr.ipv4.dstAddr },
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
            hdr.coordinate_metadata.sid,
            hdr.coordinate_metadata.task_id,
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
) 
main;
