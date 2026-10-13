# Iteration-one caller, numerical and delivery review

2026-10-13. Independent caller/oracle lane. Reviewed the current public header,
Python wrapper, CPU/CUDA evaluator, sampler and native benchmark against the
accepted plan. This report records source review; execution results belong to
the qualification artifacts produced on the supported execution host.

No remaining blocking finding was identified. The later sampler finite-output
and raw-record-tail findings described below were corrected by their owners
and re-read in this source review.

The CPU implementation differentiates Bernstein basis functions; the CUDA
implementation differentiates homogeneous control nets and uses tensor de
Casteljau. Both use the prescribed ordinary-derivative jet ordering and the
full request count as the component-major stride. Zero-degree branches do not
read undefined derivative coefficients. Mixed recovery subtracts both
denominator cross terms, and second pure recovery includes the factor two.
The exact oracle independently solves the general product identity with
`Fraction` over the supplied binary64 inputs; it does not certify one evaluator
by comparison with the other. Invalid slots are checked before patch reads;
failed requests overwrite every requested component with NaN.

The wrapper's immutable `Patch` converts caller controls and frame/UV metadata
to owned tuples. `Lineage` is a frozen value with full body/kind/entity/revision/
chart fields. Uploaded queries bind the native patch snapshot rather than a
later selectable patch set. Downloaded arrays are separate mutable host values;
their mutation does not edit resident results. Downloaded source descriptors,
slots and UVs are independently retained, so closing public native handles
does not lose conversion or lineage metadata. The private descriptor table is
not a public mutable CAD topology map, and no native pointer identity crosses
the Python/C boundary as geometry identity.

`JetBatch.convert()` copies the mutable downloaded data/status arrays before
the GIL-released native call. It transforms the actual supplied local doubles,
preserves prior per-query failures, and explicitly refuses frames or jet
magnitudes outside the stated helper envelope. Its source-UV factors are
affine rectangle factors only; no imported nonlinear chart is implied.
Conversion rejects a second conversion on an already converted batch. Public
result information is diagnostic metadata; editing its ctypes fields does not
replace the independently retained source descriptors or native query binding.

The sampler now checks both first derivatives against the explicit rational
cylinder map, checks positions and the cylinder locus, and writes exact input
bits, statuses and a recipe. Its output is correctly described as an untrimmed
carrier point/grid sample, with no face containment or chord-tolerance claim.
The benchmark's expanded cubic truth is independent of the evaluator. Its
resident operation includes result allocation/completion; download and cleanup
are reported separately. Prepared-host timing includes upload, evaluate,
download and status consumption, while analytic qualification remains outside
the timed region. Kernel profiling is a separately labeled context. The runner
now includes the 32-query case and seven samples from the accepted plan.

Two earlier delivery findings were resolved before this pass: the sampler
previously requested derivatives without checking them or preserving its input
recipe; the runner previously defaulted to five samples and omitted the
32-query case. A cosmetic grid-line-count comment was also reported to the
integrator for correction.

The sampler needed an explicit finite-output check before taking maxima: Python
comparisons with NaN can leave a previously finite maximum unchanged, and NaN
does not exceed a tolerance. The numerical implementation currently rejects
nonfinite successful outputs, but the sampler's own independent qualification
must reject that fault directly. The integrator added the finite check; the
native benchmark and exact caller oracle already reject it. The runtime review
also identified incomplete homogeneous
record tails at the raw C boundary; a five-double, degree-zero rejection case
was added to exercise the centrally corrected behavior on either backend.
Re-reading the core confirms that incomplete records are rejected before
backend selection; re-reading the sampler confirms the finite check precedes
error maxima. The grid-line selection now enforces its stated maximum.

Limitations remain explicit: numerical targets are qualified on a finite
corpus, not a universal floating-point error theorem; the first sampler does
not consume imported trimmed CAD faces; no SSI, Boolean, topology editing or
existing kernel evaluator replacement is implemented. CUDA lifecycle/fault
classification requires native fault and sanitizer checks in addition to the
geometric oracle. Requested CUDA qualification must fail when unavailable,
without a CPU fallback or success-by-skip.

## Final caller execution and second sampler review

The integrator reported the final supported-host runs: all 27 public cases
passed on each CUDA device; the CPU run passed with its one explicitly
GPU-only device-selection case skipped. Both updated Compute Sanitizer suites
reported zero errors and zero leaks. CPU and both CUDA exact qualifiers each
passed 1,749 requests and 19,170 direct component checks. These are execution
results reported by the integrator, distinct from this source review.

Re-reviewed the sampler's added `--shape polynomial` branch. Its sixteen
unit-weight cubic controls encode
`g(s) = 3s/4 + 3s²/4 - s³/2`, `g(t)`, and
`z = 9s(1-s)t(1-t)/2`. The independently expanded position and first-derivative
expressions match this carrier: `g'(s) = 3/4 + 3s/2 - 3s²/2`,
`zs = 9(1-2s)t(1-t)/2`, and `zt = 9s(1-s)(1-2t)/2`.
The recipe writes degree `(3,3)` and the controls table now derives its record
count from the actual payload, preserving all sixteen records. Its locus field
is explicitly not applicable. The rational cylinder branch retains its six
controls, degree `(2,1)`, analytic map, derivatives and locus check. Both
branches reject nonfinite output before maxima, retain exact query/control
bits, and label the output as an untrimmed carrier grid. No new finding in this
pass. The integrator still owns final execution of both sampler branches.
