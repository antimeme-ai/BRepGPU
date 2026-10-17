// Generated from owned P128 tables; exact Fraction conversion.
#ifndef BREPGPU_ADAPTIVE_RN_TABLES_HPP
#define BREPGPU_ADAPTIVE_RN_TABLES_HPP
namespace bgmesh::adaptive::numeric {
BGAD_HD inline Interval half_pi() { return {0x1.921fb54442d18p+0,0x1.921fb54442d19p+0}; }
BGAD_HD inline uint32_t sine_count() { return 8; }
BGAD_HD inline Interval sine_coefficient(uint32_t i) { switch(i) {
case 0: return {0x1.fffffffffffffp-1,0x1.0000000000000p+0};
case 1: return {-0x1.5555555555556p-3,-0x1.5555555555555p-3};
case 2: return {0x1.1111111111111p-7,0x1.1111111111112p-7};
case 3: return {-0x1.a01a01a01a01bp-13,-0x1.a01a01a01a01ap-13};
case 4: return {0x1.71de3a556c733p-19,0x1.71de3a556c734p-19};
case 5: return {-0x1.ae64567f544dfp-26,-0x1.ae64567f544dep-26};
case 6: return {0x1.61246139ca78ep-33,0x1.61246139ca78fp-33};
case 7: return {-0x1.ae7d8ff3fd0ecp-41,-0x1.ae7d8ff3fd0ebp-41};
default: return {0.,0.}; } }
BGAD_HD inline double sine_error() { return 0x1.e100000000000p-150; }
BGAD_HD inline Interval sine_anchor(uint32_t i) { switch(i) {
case 0: return {0x0.0p+0,0x0.0p+0};
case 1: return {0x1.ffeaaaeeee86ep-6,0x1.ffeaaaeeee86fp-6};
case 2: return {0x1.ffaaaeeed4edap-5,0x1.ffaaaeeed4edbp-5};
case 3: return {0x1.7f701032550e4p-4,0x1.7f701032550e5p-4};
case 4: return {0x1.feaaeee86ee35p-4,0x1.feaaeee86ee36p-4};
case 5: return {0x1.3eb312c5d66cbp-3,0x1.3eb312c5d66ccp-3};
case 6: return {0x1.7dc102fbaf2b5p-3,0x1.7dc102fbaf2b6p-3};
case 7: return {0x1.bc6f84edc6199p-3,0x1.bc6f84edc619ap-3};
case 8: return {0x1.faaeed4f31576p-3,0x1.faaeed4f31577p-3};
case 9: return {0x1.1c37d64c6b876p-2,0x1.1c37d64c6b877p-2};
case 10: return {0x1.3ad129769d3d8p-2,0x1.3ad129769d3d9p-2};
case 11: return {0x1.591bc9fa2f597p-2,0x1.591bc9fa2f598p-2};
case 12: return {0x1.7710255764213p-2,0x1.7710255764214p-2};
case 13: return {0x1.94a6be9f546c4p-2,0x1.94a6be9f546c5p-2};
case 14: return {0x1.b1d8305321616p-2,0x1.b1d8305321617p-2};
case 15: return {0x1.ce9d2e3d4a51ep-2,0x1.ce9d2e3d4a51fp-2};
case 16: return {0x1.eaee8744b05efp-2,0x1.eaee8744b05f0p-2};
case 17: return {0x1.0362939c69954p-1,0x1.0362939c69955p-1};
case 18: return {0x1.110d0c4b69c3bp-1,0x1.110d0c4b69c3cp-1};
case 19: return {0x1.1e7343236574cp-1,0x1.1e7343236574dp-1};
case 20: return {0x1.2b91dea88421dp-1,0x1.2b91dea88421ep-1};
case 21: return {0x1.386597456282ap-1,0x1.386597456282bp-1};
case 22: return {0x1.44eb381cf386ap-1,0x1.44eb381cf386bp-1};
case 23: return {0x1.511f9fd7b351bp-1,0x1.511f9fd7b351cp-1};
case 24: return {0x1.5cffc16bf8f0dp-1,0x1.5cffc16bf8f0ep-1};
case 25: return {0x1.6888a4e134b2ep-1,0x1.6888a4e134b2fp-1};
case 26: return {0x1.73b7680dea577p-1,0x1.73b7680dea578p-1};
case 27: return {0x1.7e893f5037959p-1,0x1.7e893f503795ap-1};
case 28: return {0x1.88fb7640b8da1p-1,0x1.88fb7640b8da2p-1};
case 29: return {0x1.930b705f9f859p-1,0x1.930b705f9f85ap-1};
case 30: return {0x1.9cb6a9bbce64ap-1,0x1.9cb6a9bbce64bp-1};
case 31: return {0x1.a5fab793d29c8p-1,0x1.a5fab793d29c9p-1};
case 32: return {0x1.aed548f090ceep-1,0x1.aed548f090cefp-1};
default: return {-1.,1.}; } }
BGAD_HD inline uint32_t cosine_count() { return 9; }
BGAD_HD inline Interval cosine_coefficient(uint32_t i) { switch(i) {
case 0: return {0x1.fffffffffffffp-1,0x1.0000000000000p+0};
case 1: return {-0x1.0000000000000p-1,-0x1.fffffffffffffp-2};
case 2: return {0x1.5555555555555p-5,0x1.5555555555556p-5};
case 3: return {-0x1.6c16c16c16c17p-10,-0x1.6c16c16c16c16p-10};
case 4: return {0x1.a01a01a01a01ap-16,0x1.a01a01a01a01bp-16};
case 5: return {-0x1.27e4fb7789f5dp-22,-0x1.27e4fb7789f5cp-22};
case 6: return {0x1.1eed8eff8d894p-29,0x1.1eed8eff8d895p-29};
case 7: return {-0x1.93974a8b49dd0p-37,-0x1.93974a8b49dcfp-37};
case 8: return {0x1.ae7da946c47eep-45,0x1.ae7da946c47efp-45};
default: return {0.,0.}; } }
BGAD_HD inline double cosine_error() { return 0x1.0000000000000p-159; }
BGAD_HD inline Interval cosine_anchor(uint32_t i) { switch(i) {
case 0: return {0x1.0000000000000p+0,0x1.0000000000000p+0};
case 1: return {0x1.ffc00155527d2p-1,0x1.ffc00155527d3p-1};
case 2: return {0x1.ff0015549f4d3p-1,0x1.ff0015549f4d4p-1};
case 3: return {0x1.fdc06bf7e6b9bp-1,0x1.fdc06bf7e6b9cp-1};
case 4: return {0x1.fc015527d5bd3p-1,0x1.fc015527d5bd4p-1};
case 5: return {0x1.f9c340a7cc428p-1,0x1.f9c340a7cc429p-1};
case 6: return {0x1.f706bdf9ece1bp-1,0x1.f706bdf9ece1cp-1};
case 7: return {0x1.f3cc7c3b3d16dp-1,0x1.f3cc7c3b3d16ep-1};
case 8: return {0x1.f01549f7deea1p-1,0x1.f01549f7deea2p-1};
case 9: return {0x1.ebe214f76efa7p-1,0x1.ebe214f76efa8p-1};
case 10: return {0x1.e733ea0193d3fp-1,0x1.e733ea0193d40p-1};
case 11: return {0x1.e20bf49acd6c0p-1,0x1.e20bf49acd6c1p-1};
case 12: return {0x1.dc6b7eb995912p-1,0x1.dc6b7eb995913p-1};
case 13: return {0x1.d653f073e403fp-1,0x1.d653f073e4040p-1};
case 14: return {0x1.cfc6cfa52ad9fp-1,0x1.cfc6cfa52ada0p-1};
case 15: return {0x1.c8c5bf8ce1a84p-1,0x1.c8c5bf8ce1a85p-1};
case 16: return {0x1.c1528065b7d4fp-1,0x1.c1528065b7d50p-1};
case 17: return {0x1.b96eeef58840ep-1,0x1.b96eeef58840fp-1};
case 18: return {0x1.b11d04162a4c6p-1,0x1.b11d04162a4c7p-1};
case 19: return {0x1.a85ed4373e02dp-1,0x1.a85ed4373e02ep-1};
case 20: return {0x1.9f368ed912f84p-1,0x1.9f368ed912f85p-1};
case 21: return {0x1.95a67e00cb1fcp-1,0x1.95a67e00cb1fdp-1};
case 22: return {0x1.8bb105a5dc900p-1,0x1.8bb105a5dc901p-1};
case 23: return {0x1.8158a31916d5cp-1,0x1.8158a31916d5dp-1};
case 24: return {0x1.769fec655211ep-1,0x1.769fec655211fp-1};
case 25: return {0x1.6b898fa9efb5dp-1,0x1.6b898fa9efb5ep-1};
case 26: return {0x1.6018526f563dfp-1,0x1.6018526f563e0p-1};
case 27: return {0x1.544f10f592ca4p-1,0x1.544f10f592ca5p-1};
case 28: return {0x1.4830bd7d4ceb3p-1,0x1.4830bd7d4ceb4p-1};
case 29: return {0x1.3bc05f8b3a656p-1,0x1.3bc05f8b3a657p-1};
case 30: return {0x1.2f011326420e4p-1,0x1.2f011326420e5p-1};
case 31: return {0x1.21f608107e379p-1,0x1.21f608107e37ap-1};
case 32: return {0x1.14a280fb5068bp-1,0x1.14a280fb5068cp-1};
default: return {-1.,1.}; } }
} // namespace bgmesh::adaptive::numeric
#endif
