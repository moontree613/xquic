
#include <xquic/xquic_typedef.h>
#include <xquic/xquic.h>
void xqc_fec_update_fec_ratio(xqc_path_ctx_t *path);
void xqc_fec_ob_wnd_update_on_packet_acked(xqc_connection_t *conn, xqc_packet_out_t *acked_packet);
void xqc_fec_ob_wnd_update_on_packet_lost(xqc_connection_t *conn, xqc_packet_out_t *acked_packet);
void xqc_fec_ob_wnd_update_on_packet_repaired(xqc_connection_t *conn, uint64_t repair_pkt_num, uint64_t path_id);
