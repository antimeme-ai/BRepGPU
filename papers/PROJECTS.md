# Projects to inspect

[Reference implementations by kernel responsibility](REFERENCE_IMPLEMENTATIONS.md)
maps mature B-rep kernels, spline/evaluator code, actual CUDA NURBS work,
arrangements and GPU execution primitives to the stages of BRepGPU. It includes
additional verified leads beyond the acquired snapshots listed below.

Collected and inspected 2026-10-13. These are references for algorithms,
contracts, workloads and interaction. Inclusion does not select a dependency,
establish correctness or imply a shared license. Source revisions and archive
digests are in the [catalog](reference-catalog.tsv) and
[BibTeX](reference-snapshots.bib).

| Project | Why inspect it | Inspected source |
| --- | --- | --- |
| [usd-brep](https://github.com/NVIDIA-Omniverse/usd-brep) | CPU B-rep kernel and USD bridge; experiment target | [c9c5979f04ce](https://github.com/NVIDIA-Omniverse/usd-brep/tree/c9c5979f04cea8c38213004d4f04df0dcfb40d05) |
| [truck](https://github.com/ricosjp/truck) | Rust B-rep topology and geometric algorithms | [88ed005249e5](https://github.com/ricosjp/truck/tree/88ed005249e5e3a6b07f62425399435905cd3ab6) |
| [warp](https://github.com/NVIDIA/warp) | GPU geometry/data-parallel machinery; separate from USD BRep | [28159649a2bb](https://github.com/NVIDIA/warp/tree/28159649a2bb8b91e1badf692007c38a9f62d6b5) |
| [geogram](https://github.com/BrunoLevy/geogram) | Exact mesh CSG and arrangement construction | [390b14d5b9df](https://github.com/BrunoLevy/geogram/tree/390b14d5b9df949c45d07d5af208700f6be3a616) |
| [manifold](https://github.com/elalish/manifold) | Mesh solid operations and numerical/topological contracts | [761661c15bac](https://github.com/elalish/manifold/tree/761661c15bacb6c8459d682c715de84591d51774) |
| [fidget](https://github.com/mkeeter/fidget) | Implicit geometry, interval pruning and execution organization | [eb4e9487cac6](https://github.com/mkeeter/fidget/tree/eb4e9487cac6570110ff0a4d8ae81ad9d8e35110) |
| [opencadkernel](https://github.com/HakanSeven12/opencadkernel) | Small Rust CAD kernel; compare its declared geometric scope | [3d77c6bd5ec2](https://github.com/HakanSeven12/opencadkernel/tree/3d77c6bd5ec2c7a380b2ae296bafd56dbe430fb4) |
| [keel](https://github.com/redwoodrhetorica/keel) | New kernel research and contracts; treat repository claims as claims | [f8b68bb2ab3f](https://github.com/redwoodrhetorica/keel/tree/f8b68bb2ab3f4c3abb41a3612d7ffcade36b34d8) |
| [vcad](https://github.com/ecto/vcad) | Young CAD kernel/application stack; inspect supported operations | [eba7a2e6a89f](https://github.com/ecto/vcad/tree/eba7a2e6a89ff06801776fbc599d5c8a64036168) |
| [brepjs](https://github.com/andymai/brepjs) | Browser CAD and B-rep-facing API patterns | [ee674d6ccbf8](https://github.com/andymai/brepjs/tree/ee674d6ccbf86499bf87cf508b32a77e4ca75ed9) |
| [kernelcad-web](https://github.com/w1ne/kernelCAD-web) | Web CAD application architecture | [3da1eb8add93](https://github.com/w1ne/kernelCAD-web/tree/3da1eb8add938c0f79361c6a8427723ce02ba59f) |
| [dune3d](https://github.com/dune3d/dune3d) | Interactive parametric modeling and constraint workflow | [f7a1bb0108ff](https://github.com/dune3d/dune3d/tree/f7a1bb0108fff67169e50329574b235b94cdbf84) |
| [chili3d](https://github.com/xiangechen/chili3d) | Browser CAD interaction and OCCT-backed modeling | [03a6a542e841](https://github.com/xiangechen/chili3d/tree/03a6a542e841a7f1952f67aa729e0658a8096c08) |
| [planegcs](https://github.com/Salusoft89/planegcs) | Sketch constraint solver mechanics | [9d26baef0b24](https://github.com/Salusoft89/planegcs/tree/9d26baef0b240fe0eef4186d4e5e5e66ec3620b7) |
| [build123d](https://github.com/gumyr/build123d) | Programmatic modeling tasks and readable fixture recipes | [f5cca68a7500](https://github.com/gumyr/build123d/tree/f5cca68a7500ec452759a463010aced0cbbd278f) |
| [cadquery](https://github.com/CadQuery/cadquery) | Programmatic modeling and interchange workloads | [5f5bc62b255f](https://github.com/CadQuery/cadquery/tree/5f5bc62b255f62e3512105740025361b6c31eddd) |

The catalog includes additional CAD applications, AI model-program research and
evaluation projects. Study their declared precision, input domain, fallback
behavior and licenses before trying to compare them. Mesh and implicit engines
are useful algorithmic references alongside surface/curve B-rep kernels.

NVIDIA USD BRep 1.0.0 is the CPU reference measured here. Its inspected modeling
source does not supply a CUDA B-rep backend. Warp is separate GPU software;
availability of Warp does not make USD BRep modeling GPU accelerated.

The [three research reports](README.md) supply the narrower numerical,
intersection/topology and correctness reading paths for this project.
