#!/usr/bin/env bash
#
# Install a pre-commit hook that runs the same gates as the route-gates CI job.
#
#   Tools/verify/install_git_hooks.sh
#
# The hook is local to this clone (git does not share hooks), which is the point: it stops
# a commit that would turn the CI red - a personal absolute path in a document, for instance
# - while it is still cheap to fix.
set -euo pipefail

ROOT=$(cd "$(dirname "$0")/../.." && pwd)
HOOK="$ROOT/.git/hooks/pre-commit"

cat > "$HOOK" <<'HOOK_BODY'
#!/usr/bin/env bash
# Installed by Tools/verify/install_git_hooks.sh - runs the route-gates checks.
ROOT=$(git rev-parse --show-toplevel)
exec "$ROOT/Tools/verify/run_gates.sh"
HOOK_BODY

chmod +x "$HOOK"
echo "installed $HOOK"
