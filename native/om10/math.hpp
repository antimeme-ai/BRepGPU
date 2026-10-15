#ifndef BG_OM10_MATH_HPP
#define BG_OM10_MATH_HPP
#include "../evaluator.hpp"
#include "types.hpp"
#include <cfloat>
#include <cmath>
#if defined(__CUDACC__)
#define BGOM_HD __host__ __device__ inline
#else
#define BGOM_HD inline
#endif
namespace bgom10 {
BGOM_HD bool finite(double x) { return brepgpu::numeric_finite(x); }
BGOM_HD Vec vec(const double *p) { return {p[0], p[1], p[2]}; }
BGOM_HD Vec add(Vec a, Vec b) { return {a.x + b.x, a.y + b.y, a.z + b.z}; }
BGOM_HD Vec sub(Vec a, Vec b) { return {a.x - b.x, a.y - b.y, a.z - b.z}; }
BGOM_HD Vec mul(Vec a, double b) { return {a.x * b, a.y * b, a.z * b}; }
BGOM_HD double dot(Vec a, Vec b) {
  return ((a.x * b.x + a.y * b.y) + a.z * b.z);
}
BGOM_HD Vec cross(Vec a, Vec b) {
  return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}
BGOM_HD bool good(Vec a) { return finite(a.x) && finite(a.y) && finite(a.z); }
BGOM_HD double maxd(double a, double b) { return a > b ? a : b; }
BGOM_HD double mind(double a, double b) { return a < b ? a : b; }
BGOM_HD bool unit(Vec a, Vec &out) {
  double m = maxd(fabs(a.x), maxd(fabs(a.y), fabs(a.z)));
  if (!finite(m) || m == 0)
    return false;
  Vec b = mul(a, 1 / m);
  double n = sqrt(dot(b, b));
  out = mul(b, 1 / n);
  return good(out);
}
BGOM_HD void save(Vec a, double *p) {
  p[0] = a.x;
  p[1] = a.y;
  p[2] = a.z;
}
BGOM_HD void basis(uint32_t n, double t, double *b, double *d) {
  double s = 1 - t;
  for (int i = 0; i < 4; ++i)
    b[i] = d[i] = 0;
  if (n == 0)
    b[0] = 1;
  if (n == 1) {
    b[0] = s;
    b[1] = t;
    d[0] = -1;
    d[1] = 1;
  }
  if (n == 2) {
    b[0] = s * s;
    b[1] = 2 * s * t;
    b[2] = t * t;
    d[0] = -2 * s;
    d[1] = 2 * (s - t);
    d[2] = 2 * t;
  }
  if (n == 3) {
    b[0] = s * s * s;
    b[1] = 3 * s * s * t;
    b[2] = 3 * s * t * t;
    b[3] = t * t * t;
    d[0] = -3 * s * s;
    d[1] = 3 * s * s - 6 * s * t;
    d[2] = 6 * s * t - 3 * t * t;
    d[3] = 3 * t * t;
  }
}
BGOM_HD bool evaluate(const Carrier &c, const Control *controls,
                      const Vertex &v, Jet &out) {
  const double u = v.uv[0], w = v.uv[1];
  Vec p{}, du{}, dv{}, normal{};
  out.status = 0;
  out.flags = 0;
  const Vec o = vec(c.origin), x = vec(c.x), y = vec(c.y), z = vec(c.z);
  if (c.tag == 1) {
    p = add(o, add(mul(x, u), mul(y, w)));
    du = x;
    dv = y;
    normal = cross(x, y);
  } else if (c.tag >= 2 && c.tag <= 5) {
    double su = sin(u), cu = cos(u);
    Vec e = add(mul(x, cu), mul(y, su)), ep = add(mul(x, -su), mul(y, cu));
    if (c.tag == 2) {
      p = add(o, add(mul(e, c.r0), mul(z, w)));
      du = mul(ep, c.r0);
      dv = z;
      normal = cross(ep, z);
    }
    if (c.tag == 3) {
      double sa = sin(c.r1), ca = cos(c.r1), r = c.r0 + w * sa;
      p = add(o, add(mul(e, r), mul(z, w * ca)));
      du = mul(ep, r);
      dv = add(mul(e, sa), mul(z, ca));
      normal = add(mul(cross(ep, e), sa), mul(cross(ep, z), ca));
      if (r < 0)
        normal = mul(normal, -1);
      if (r == 0)
        out.flags |= 1;
    }
    if (c.tag == 4) {
      double sv = sin(w), cv = cos(w);
      p = add(o, add(mul(e, c.r0 * cv), mul(z, c.r0 * sv)));
      du = mul(ep, c.r0 * cv);
      dv = add(mul(e, -c.r0 * sv), mul(z, c.r0 * cv));
      normal = add(mul(cross(ep, e), -sv), mul(cross(ep, z), cv));
      if (cv < 0)
        normal = mul(normal, -1);
      if (fabs(cv) < 1e-14)
        out.flags |= 1;
    }
    if (c.tag == 5) {
      double sv = sin(w), cv = cos(w), r = c.r0 + c.r1 * cv;
      p = add(o, add(mul(e, r), mul(z, c.r1 * sv)));
      du = mul(ep, r);
      dv = add(mul(e, -c.r1 * sv), mul(z, c.r1 * cv));
      normal = add(mul(cross(ep, e), -sv), mul(cross(ep, z), cv));
      if (r < 0)
        normal = mul(normal, -1);
      if (r == 0)
        out.flags |= 1;
    }
  } else if (c.tag == 6) {
    double a = (u - c.uv[0]) / (c.uv[1] - c.uv[0]),
           b = (w - c.uv[2]) / (c.uv[3] - c.uv[2]);
    double bu[4], bv[4], dbu[4], dbv[4];
    basis(c.p, a, bu, dbu);
    basis(c.q, b, bv, dbv);
    double h[3][4]{};
    for (uint32_t i = 0; i <= c.p; ++i)
      for (uint32_t j = 0; j <= c.q; ++j) {
        const Control &hcp = controls[c.control_offset + i * (c.q + 1) + j];
        double f[3] = {bu[i] * bv[j], dbu[i] * bv[j], bu[i] * dbv[j]};
        for (int k = 0; k < 3; ++k)
          for (int axis = 0; axis < 4; ++axis)
            h[k][axis] += f[k] * hcp.h[axis];
      }
    double weights[6] = {h[0][3], h[1][3], h[2][3], 0, 0, 0};
    double values[3][3];
    for (int axis = 0; axis < 3; ++axis) {
      double numerator[6] = {h[0][axis], h[1][axis], h[2][axis], 0, 0, 0},
             r[6]{};
      if (!brepgpu::recover_channel(numerator, weights, 1, r))
        return false;
      for (int k = 0; k < 3; ++k)
        values[k][axis] = r[k];
    }
    p = vec(values[0]);
    du = mul(vec(values[1]), 1 / (c.uv[1] - c.uv[0]));
    dv = mul(vec(values[2]), 1 / (c.uv[3] - c.uv[2]));
    normal = cross(du, dv);
  } else
    return false;
  if (!good(p) || !good(du) || !good(dv) || !unit(normal, normal))
    return false;
  if (v.flags & 2)
    normal = mul(normal, -1);
  save(p, out.position);
  save(du, out.du);
  save(dv, out.dv);
  save(normal, out.normal);
  save((v.flags & 1) ? vec(v.xyz) : p, out.used);
  return good(vec(out.used));
}
BGOM_HD Vec rotate(const double *m, Vec p) {
  return {dot(vec(m), p), dot(vec(m + 4), p), dot(vec(m + 8), p)};
}
BGOM_HD bool project(const SceneView &s, const Request &r, uint64_t slot,
                     const Jet *jets, Projected &out) {
  const VertexRef ref = s.vertex_refs[slot];
  const Instance &i = s.instances[ref.instance];
  const Definition &d = s.definitions[i.definition];
  Vec p = rotate(i.matrix, vec(jets[ref.source].used));
  p = add(p, {i.matrix[3], i.matrix[7], i.matrix[11]});
  if (r.alpha != 0)
    p = add(p, mul(vec(i.explosion), r.alpha));
  Vec n = rotate(i.matrix, vec(jets[ref.source].normal));
  if (!good(p) || !unit(n, n))
    return false;
  Vec v = sub(p, vec(r.center));
  double px = (r.pixel_center[0] + r.scale * dot(vec(r.basis), v)) *
              r.supersample,
         py = (r.pixel_center[1] - r.scale * dot(vec(r.basis + 3), v)) *
              r.supersample,
         z = dot(vec(r.basis + 6), v);
  double sx = px >= 0 ? floor(px * 256 + .5) : ceil(px * 256 - .5),
         sy = py >= 0 ? floor(py * 256 + .5) : ceil(py * 256 - .5);
  if (!finite(sx) || !finite(sy) || fabs(sx) > 536870912. ||
      fabs(sy) > 536870912. || !finite(z))
    return false;
  out.instance = i.id;
  out.definition = d.id;
  out.source_vertex = ref.source;
  out.x = static_cast<int64_t>(sx);
  out.y = static_cast<int64_t>(sy);
  out.depth = z;
  save(p, out.world);
  save(n, out.normal);
  return true;
}
BGOM_HD int64_t edge(int64_t ax, int64_t ay, int64_t bx, int64_t by, int64_t px,
                     int64_t py) {
  return (bx - ax) * (py - ay) - (by - ay) * (px - ax);
}
BGOM_HD bool top_left(int64_t ax, int64_t ay, int64_t bx, int64_t by) {
  return by < ay || (by == ay && bx > ax);
}
BGOM_HD bool setup(const SceneView &s, const Request &r, uint64_t slot,
                   const Projected *points, Prepared &out) {
  PrimitiveRef ref = s.primitive_refs[slot];
  const Triangle &t = s.triangles[ref.triangle];
  out = {};
  out.primitive = slot;
  out.instance = s.instances[ref.instance].id;
  out.material = t.material;
  for (int k = 0; k < 3; ++k) {
    out.vertex[k] = ref.vertex_base + t.vertex[k];
    const Projected &p = points[out.vertex[k]];
    out.x[k] = p.x;
    out.y[k] = p.y;
    out.z[k] = p.depth;
  }
  out.area = edge(out.x[0], out.y[0], out.x[1], out.y[1], out.x[2], out.y[2]);
  if (out.area == 0)
    return true;
  if (out.area < 0) {
    for (int k = 0; k < 1; ++k) {
      int64_t a = out.x[1];
      out.x[1] = out.x[2];
      out.x[2] = a;
      a = out.y[1];
      out.y[1] = out.y[2];
      out.y[2] = a;
    }
    double z = out.z[1];
    out.z[1] = out.z[2];
    out.z[2] = z;
    uint32_t v = out.vertex[1];
    out.vertex[1] = out.vertex[2];
    out.vertex[2] = v;
    out.area = -out.area;
  }
  double xmin = mind(out.x[0], mind(out.x[1], out.x[2])),
         xmax = maxd(out.x[0], maxd(out.x[1], out.x[2])),
         ymin = mind(out.y[0], mind(out.y[1], out.y[2])),
         ymax = maxd(out.y[0], maxd(out.y[1], out.y[2]));
  out.xmin = static_cast<int32_t>(maxd(0, ceil((xmin - 128) / 256)));
  out.xmax = static_cast<int32_t>(
      mind(r.width * r.supersample - 1, floor((xmax - 128) / 256)));
  out.ymin = static_cast<int32_t>(maxd(0, ceil((ymin - 128) / 256)));
  out.ymax = static_cast<int32_t>(
      mind(r.height * r.supersample - 1, floor((ymax - 128) / 256)));
  if (out.xmin > out.xmax || out.ymin > out.ymax)
    out.area = 0;
  return true;
}
BGOM_HD bool covered(const Prepared &t, uint32_t x, uint32_t y, double *w,
                     double &z) {
  int64_t px = static_cast<int64_t>(x) * 256 + 128,
          py = static_cast<int64_t>(y) * 256 + 128;
  for (int k = 0; k < 3; ++k) {
    int a = (k + 1) % 3, b = (k + 2) % 3;
    int64_t e = edge(t.x[a], t.y[a], t.x[b], t.y[b], px, py);
    if (e < 0 || (e == 0 && !top_left(t.x[a], t.y[a], t.x[b], t.y[b])))
      return false;
    w[k] = static_cast<double>(e) / static_cast<double>(t.area);
  }
  z = ((w[0] * t.z[0] + w[1] * t.z[1]) + w[2] * t.z[2]);
  return true;
}
BGOM_HD double power(double x, unsigned exponent) {
  double y = 1;
  while (exponent) {
    if (exponent & 1)
      y *= x;
    x *= x;
    exponent >>= 1;
  }
  return y;
}
BGOM_HD bool shade(const Prepared &t, const Projected *p,
                   const Material *materials, const Request &r, const double *w,
                   Vec &color) {
  Vec n = add(add(mul(vec(p[t.vertex[0]].normal), w[0]),
                  mul(vec(p[t.vertex[1]].normal), w[1])),
              mul(vec(p[t.vertex[2]].normal), w[2]));
  if (!unit(n, n))
    return false;
  n = {dot(vec(r.basis), n), dot(vec(r.basis + 3), n),
       dot(vec(r.basis + 6), n)};
  Vec light{}, half{}, fill{};
  unit({-.35, .50, .79}, light);
  unit(add(light, {0, 0, 1}), half);
  unit({.65, -.4, .55}, fill);
  double diffuse = maxd(0, dot(n, light)), f = maxd(0, dot(n, fill));
  const Material &m = materials[t.material];
  unsigned exponent =
      m.roughness > .65
          ? 16
          : (m.roughness > .35 ? 32 : (m.roughness > .15 ? 64 : 128));
  double spec = .55 * power(maxd(0, dot(n, half)), exponent),
         gain = .20 + .70 * diffuse + .16 * f;
  Vec base = vec(m.rgb);
  color = {base.x * gain + spec * (.4 + .6 * base.x),
           base.y * gain + spec * (.4 + .6 * base.y),
           base.z * gain + spec * (.4 + .6 * base.z)};
  return good(color);
}
BGOM_HD uint8_t encode(double x) {
  x = maxd(0, mind(1, x));
  double y = x <= .0031308 ? 12.92 * x : 1.055 * pow(x, 1 / 2.4) - .055;
  return static_cast<uint8_t>(floor(255 * y + .5));
}
} // namespace bgom10
#undef BGOM_HD
#endif
