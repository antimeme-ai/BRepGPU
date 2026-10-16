# Shared finite display boundary validation

Replace the decoder-only record checks and incomplete direct-API checks with one
borrowed SceneView validator. It reads existing arrays without copying geometry;
PMR scratch stores only sorted definition/instance IDs to reject duplicates.
A validation context supplies the scratch resource and optional work-charge
callback. Native SceneStore uses its caller Budget for both; old direct APIs and
file parsing use their existing host resource. No record or ABI layout changes.

Separate shared functions validate base records and canonical flat placements:
the decoder validates records, builds its existing placement arrays, then checks
the same placement rules; direct CPU/CUDA creation and make_scene validate both.
Checks preserve the existing finite source/legacy profiles, complete definition
partitions, finite coefficients/frames, source-normal flags, local triangle
indices/noncollapse, material inheritance, IDs and exact flattened placements.
Source/approximation correctness remains the native mesher's responsibility.

One Request validator enforces the existing file contract (all finite fields,
dimensions, flags/reserved, alpha/scale, camera basis and background). File reader,
request writer and direct CPU/CUDA render call it before work. Invalid requests
fail before output mutation; invalid scenes fail before a session is created.

Author direct malformed scene/request and writer-refusal cases first, observe
old implementation RED on the fleet, then implement. Check actual CPU/both P100
admission/render and affected legacy compatibility plus a relevant sanitizer
fixture; inspect borrowed-array bounds under peer review. No driver fault or
geometry certification campaign.
