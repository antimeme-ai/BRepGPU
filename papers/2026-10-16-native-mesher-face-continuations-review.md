# Native face preparation and refinement continuation review

Scope: the actual `face_preparation_job.hpp/.cpp` and
`face_job.hpp/.cpp`, including final face export, optimizer commands and
contact horizons. Read-only review; no execution campaign.

One concrete admission finding was sent to the owner: `FaceOwnerJob`
accepted a nonnull RunResource different from its retained boundary's
resource. Its mutable arithmetic could therefore be reserved against one
run while published packets/horizons/results allocated through another.
Require equality at constructor admission, as `FacePreparationJob`
already does. The immutable metric table is imported into the private
owner budget; its table-key semantics are retained separately.

Otherwise the inspected continuation mechanism is constructive:

- Each preparation/refinement owner retains one prepaid private arena,
  geometry context and cancellation token; child packets contain frozen
  values and leases, with no borrowed Rational crossing a scheduler yield.
  Engine/preparation objects are destroyed before their Budget and lease.
- Numerical replies are validated as a complete unique-key set with all
  source/boundary/face/triangle/request/policy metadata before mutation.
  The engine receives only complete batches; canonical frontier priority
  remains the engine's ordering rather than reply arrival order.
- Preparation yields finite curve batches or the canonical boundary
  request. It cannot silently resume an old horizon. The coordinator owns
  all-face barrier closure/replacement; these wrappers do not select a
  completion-dependent request subset.
- Contact snapshots carry physical identity/XYZ and full cell identity,
  clearing unleased parameter references. Removed transactions and newly
  accepted cells are emitted through immutable, resource-owned horizons.
  Invalid mark generations/duplicates are rejected before queuing.
- Optimizer commands are mutually exclusive, retain their exact proposal,
  and require the same two accepted bound headers plus matching contact
  approval before commit. The coordinator still owns the global stable
  edge queue and current-definition contact authority.
- Final export is requested only from a settled/idle horizon; engine
  export independently refuses pending candidate, closure, boundary,
  optimization or frontier work and freezes actual sites, original UV
  locators, full lineage, live triangles and traversal adapters.
- Exceptions poison the retained generation. Partial state is not
  reported as a finished face. Candidate point tasks use scheduler-private
  scratch and a distinct geometry task owner.

The owner applied the admission fix and it has been read back: constructor
requires the same boundary resource, metric ExactTable resource and retained
geometry source resource as the supplied RunResource.

Verdict: scoped ACCEPT. This review does not assert correctness of every geometry or
topology algorithm called by these wrappers.
