# Tyrargo repository workflow

Preserve Rebellion/upstream/third-party notices. English is the primary documentation language; keep Spanish pages in sync. Preserve legacy source bytes/encoding unless a scoped conversion is explicitly required.

Work locally. Do not push, publish a release, change repository visibility, send upstream messages or alter an installed game unless the user explicitly requests that action. Documentation/build scripts never perform those operations.

For each implemented iteration, update the version header deliberately, add/update consolidated feature records and milestone/roadmap data, run tools/update_docs.py, --check and tools/check_docs.py. Count consolidated changes once; diagnostics and repeated experiments are not separate features. Distinguish implemented, validated and planned work. Avoid absolute performance/bug-free claims.

Validate appropriate behavior, graphics, save/load and resource lifetime for engine changes. Record compilation timestamps and executable hashes. Keep commercial assets, personal saves/profiles and diagnostic logs out of commits. Complete distribution-license review before public binary releases.
