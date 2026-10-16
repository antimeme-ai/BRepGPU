#include "source.hpp"
#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <limits>
#include <span>
#include <stdexcept>
#include <string_view>
#include <vector>

namespace {
using Bytes = std::vector<std::byte>;
void require(bool condition, const char *message) {
  if (!condition)
    throw std::runtime_error(message);
}
void word(Bytes &out, uint64_t value, uint32_t width = 8) {
  for (uint32_t i = 0; i < width; ++i)
    out.push_back(static_cast<std::byte>((value >> (8 * i)) & 255));
}
void text(Bytes &out, std::string_view value) {
  word(out, value.size(), 4);
  for (char c : value)
    out.push_back(static_cast<std::byte>(static_cast<unsigned char>(c)));
}
void append(Bytes &out, std::span<const std::byte> data) {
  out.insert(out.end(), data.begin(), data.end());
}
void number(Bytes &out, double value) { word(out, std::bit_cast<uint64_t>(value)); }
void row(Bytes &out, const Bytes &value) { word(out, value.size()); append(out, value); }
Bytes fixture(bool curve = false, bool bad_weight = false, bool cycle = false) {
  std::array<Bytes, 13> payloads;
  std::array<uint64_t, 13> counts{};
  Bytes value;
  text(value, "native source fixture"); row(payloads[0], value); counts[0] = 1;
  value.clear(); word(value, 101); word(value, 0); word(value, 0);
  text(value, "root"); text(value, "drawing"); row(payloads[1], value); counts[1] = 1;
  if (cycle) {
    value.clear(); word(value, 99); word(value, 101); word(value, 101); word(value, 0);
    text(value, "cycle");
    for (double x : std::array<double, 12>{1,0,0,0,0,1,0,0,0,0,1,0}) number(value,x);
    row(payloads[3], value); counts[3] = 1;
  }
  if (curve) {
    value.clear(); word(value, 103); word(value, 3, 4); word(value, 4, 4);
    for (unsigned i = 0; i < 14; ++i) number(value, 0);
    number(value, 0); number(value, 1); word(value, 1, 4); word(value, 0, 4);
    word(value, 0); word(value, 0); word(value, 8);
    for (double x : std::array<double, 8>{-0.0,0,0,1,1,2,3,bad_weight ? 0.0 : 2.0})
      number(value,x);
    word(value, 0); row(payloads[10],value); counts[10]=1;
  }
  constexpr std::array<std::string_view,13> tags{
    "META____","PRODUCTS","SOLIDS__","USAGES__","VERTICES","EDGES___","COEDGES_",
    "LOOPS___","FACES___","CARRIERS","CURVES__","STYLES__","DIAGS___"};
  Bytes out;
  for (char c : std::string_view("BGSRC001")) out.push_back(static_cast<std::byte>(c));
  word(out,1,4); word(out,0,4);
  for (unsigned i=0;i<32;++i) out.push_back(static_cast<std::byte>(i));
  number(out,25.4); number(out,.001); word(out,101); word(out,13,4); word(out,0,4);
  uint64_t size=88;
  for (const auto &p : payloads) size+=24+p.size();
  word(out,size);
  for (std::size_t i=0;i<tags.size();++i) {
    for(char c:tags[i]) out.push_back(static_cast<std::byte>(c));
    word(out,counts[i]); word(out,payloads[i].size()); append(out,payloads[i]);
  }
  return out;
}
void rejected(const Bytes &bytes, bgmesh::ErrorCode expected, bgmesh::SourceLimits limits = {}) {
  auto resource=std::make_shared<bgmesh::RunResource>(uint64_t{1}<<24);
  try {
    static_cast<void>(bgmesh::read_source(bytes,resource,limits));
  } catch(const bgmesh::Error &error) {
    require(error.code==expected,"wrong source failure class");
    require(resource->live()==0,"failed source retains reserved memory");
    return;
  }
  throw std::runtime_error("invalid source admitted");
}
void check() {
  auto resource=std::make_shared<bgmesh::RunResource>(uint64_t{1}<<24);
  const auto bytes=fixture(true);
  {
    const auto source=bgmesh::read_source(bytes,resource);
    const auto &data=source.view();
    require(data.products.size()==1 && data.curves.size()==1,"lost source inventory");
    require(data.units_mm==25.4,"source metadata was rescaled/rejected");
    require(data.text(data.products[0].name)=="root","wrong source string");
    require(data.root_product_id.value==101 && data.product_index.find({101}).value==0,
            "source ID/slot association changed");
    require(data.original_bytes.size()==bytes.size(),"source bytes lost");
    require(std::equal(bytes.begin(),bytes.end(),data.original_bytes.begin()),"source bits changed");
    const auto controls=data.view(data.curves[0].controls);
    require(controls.size()==8 && std::bit_cast<uint64_t>(controls[0])==(uint64_t{1}<<63),
            "original signed-zero/control words lost");
    require(data.edge_coedges.offsets.size()==1,"empty CSR sentinel missing");
    require(resource->live()>0,"source allocations bypass run resource");
  }
  require(resource->live()==0,"source destruction leaks reserved memory");
  rejected(fixture(true,true),bgmesh::ErrorCode::invalid_source);
  rejected(fixture(false,false,true),bgmesh::ErrorCode::invalid_source);
  rejected(bytes,bgmesh::ErrorCode::resource_exhausted,{bytes.size()-1,10000,10000});
  rejected(bytes,bgmesh::ErrorCode::resource_exhausted,{uint64_t{1}<<30,0,10000});
  for (std::size_t length=0;length<bytes.size();++length) {
    Bytes cut(bytes.begin(),bytes.begin()+static_cast<std::ptrdiff_t>(length));
    rejected(cut,bgmesh::ErrorCode::invalid_source);
  }
  for(std::size_t offset:std::array<std::size_t,6>{0,8,12,72,76,80}) {
    auto corrupt=bytes; corrupt[offset]^=std::byte{0x80};
    rejected(corrupt,bgmesh::ErrorCode::invalid_source);
  }
  auto extra=bytes; extra.push_back(std::byte{0}); rejected(extra,bgmesh::ErrorCode::invalid_source);
  auto small=std::make_shared<bgmesh::RunResource>(1);
  try { static_cast<void>(bgmesh::read_source(bytes,small)); }
  catch(const bgmesh::Error &error) {
    require(error.code==bgmesh::ErrorCode::resource_exhausted && small->live()==0,
            "allocation failure is not transactional");
    return;
  }
  throw std::runtime_error("source allocation limit ignored");
}
} // namespace
int main() {
  try { check(); std::cout<<"native source specification checks PASS\n"; return 0; }
  catch(const std::exception &error) { std::cerr<<error.what()<<'\n'; return 1; }
}
