"""Generate RN binary64 enclosures of our owned P128 trig tables."""
from fractions import Fraction as Q
from pathlib import Path
import math
import sys
import native_mesher_point_trig_vectors as original


def main():
    def directed(value, upward):
        result = float(value)
        if (Q.from_float(result) < value if upward else Q.from_float(result) > value):
            result = math.nextafter(result, math.inf if upward else -math.inf)
        return result.hex()
    def interval(value):
        lo, hi = value if isinstance(value, tuple) else (value, value)
        # Same 160-bit outward coefficient/anchor grid as the owned P128 table.
        lo = original.grid(lo, 160)
        hi = original.grid(hi, 160, True)
        return '{'+directed(lo,False)+','+directed(hi,True)+'}'
    lines = ['// Generated from owned P128 tables; exact Fraction conversion.',
             '#ifndef BREPGPU_ADAPTIVE_RN_TABLES_HPP',
             '#define BREPGPU_ADAPTIVE_RN_TABLES_HPP',
             'namespace bgmesh::adaptive::numeric {']
    pi = original.pi_interval(192)
    lines += ['BGAD_HD inline Interval half_pi() { return '+interval((pi[0]/2,pi[1]/2))+'; }']
    for cosine, name in ((False,'sine'),(True,'cosine')):
        coeff, error = original.polynomial(128,cosine,True)
        lines += [f'BGAD_HD inline uint32_t {name}_count() {{ return {len(coeff)}; }}',
                  f'BGAD_HD inline Interval {name}_coefficient(uint32_t i) {{ switch(i) {{']
        lines += [f'case {i}: return '+interval(c)+';' for i,c in enumerate(coeff)]
        lines += ['default: return {0.,0.}; } }',
                  f'BGAD_HD inline double {name}_error() {{ return '+directed(original.grid(error,160,True),True)+'; }',
                  f'BGAD_HD inline Interval {name}_anchor(uint32_t i) {{ switch(i) {{']
        lines += [f'case {i}: return '+interval(original.series(Q(i,32),cosine,192))+';' for i in range(33)]
        lines += ['default: return {-1.,1.}; } }']
    lines += ['} // namespace bgmesh::adaptive::numeric','#endif']
    Path(sys.argv[1]).write_text('\n'.join(lines)+'\n')

if __name__ == '__main__':
    main()
