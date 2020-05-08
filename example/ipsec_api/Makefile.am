include $(top_srcdir)/example/Makefile.inc

bin_PROGRAMS = odp_ipsec_api

dist_check_SCRIPTS = \
		   ipsec_api_example.sh \
		   odp_ipsec_api_run_ah_in.sh \
		   odp_ipsec_api_run_ah_out.sh \
		   odp_ipsec_api_run_ah_tun_in.sh \
		   odp_ipsec_api_run_ah_tun_out.sh \
		   odp_ipsec_api_run_esp_in.sh \
		   odp_ipsec_api_run_esp_out.sh \
		   odp_ipsec_api_run_esp_tun_in.sh \
		   odp_ipsec_api_run_esp_tun_out.sh \
		   odp_ipsec_api_run_live.sh \
		   odp_ipsec_api_run_router.sh \
		   odp_ipsec_api_run_simple.sh

odp_ipsec_api_SOURCES = \
		    odp_ipsec.c \
		    odp_ipsec_sa_db.c \
		    odp_ipsec_sa_db.h \
		    odp_ipsec_sp_db.c \
		    odp_ipsec_sp_db.h \
		    odp_ipsec_fwd_db.c \
		    odp_ipsec_fwd_db.h \
		    odp_ipsec_cache.c \
		    odp_ipsec_cache.h \
		    odp_ipsec_misc.h

if WITH_OPENSSL
odp_ipsec_api_SOURCES += \
		    odp_ipsec_stream.c \
		    odp_ipsec_stream.h

AM_CPPFLAGS = $(OPENSSL_CPPFLAGS)
LDADD += $(OPENSSL_LIBS)
TESTS = ipsec_api_example.sh

else
AM_CPPFLAGS = -DNO_OPENSSL
endif
