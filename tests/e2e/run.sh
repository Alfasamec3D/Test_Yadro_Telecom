
set -u  # error on unset variables; we DON'T set -e because we want
        # to keep going even if one case fails

# args
if [[ $# -lt 1 ]]; then
    echo "usage: $0 <path-to-main-binary> [case-name]" >&2
    exit 2
fi
BIN="$1"
FILTER="${2:-}"

if [[ ! -x "$BIN" ]]; then
    echo "error: '$BIN' is not an executable file" >&2
    exit 2
fi
# Canonicalize so it works from any cwd later
BIN="$(cd "$(dirname "$BIN")" && pwd)/$(basename "$BIN")"

# ./io lives next to this script
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
CASES_DIR="$SCRIPT_DIR/io"

if [[ ! -d "$CASES_DIR" ]]; then
    echo "error: no cases directory at $CASES_DIR" >&2
    exit 2
fi

# colors (only if stdout is a tty)
if [[ -t 1 ]]; then
    RED=$'\033[0;31m'; GREEN=$'\033[0;32m'; YELLOW=$'\033[0;33m'; NC=$'\033[0m'
else
    RED=""; GREEN=""; YELLOW=""; NC=""
fi

# Normalize a file for comparison: strip CR, strip trailing whitespace per line,
# trim trailing blank lines, ensure exactly one final newline
normalize() {
    # $1 = source path, $2 = dest path
    sed -e 's/\r$//' -e 's/[[:space:]]*$//' "$1" \
        | awk 'NF || p { print; p=1 } END { }' \
        > "$2"
}

run_case() {
    local case_dir="$1"
    local name; name="$(basename "$case_dir")"
    local in="$case_dir/in.txt"
    local expected="$case_dir/expected.txt"

    if [[ ! -f "$in" || ! -f "$expected" ]]; then
        echo "${YELLOW}SKIP${NC} $name  (missing in.txt or expected.txt)"
        return 2
    fi

    local tmp; tmp="$(mktemp -d)"
    # Always clean up the temp dir, even on failure
    trap 'rm -rf "$tmp"' RETURN

    cp "$in" "$tmp/in.txt"
    (cd "$tmp" && "$BIN" in.txt) >/dev/null 2>&1
    local rc=$?

    if [[ ! -f "$tmp/result.txt" ]]; then
        echo "${RED}FAIL${NC} $name  (binary exited $rc, no result.txt produced)"
        return 1
    fi

    # Normalize both sides before comparing
    normalize "$tmp/result.txt" "$tmp/got.norm"
    normalize "$expected"       "$tmp/exp.norm"

    if diff -q "$tmp/got.norm" "$tmp/exp.norm" >/dev/null; then
        echo "${GREEN}PASS${NC} $name"
        return 0
    else
        echo "${RED}FAIL${NC} $name"
        echo "--- diff (expected vs got) ---"
        diff -u "$tmp/exp.norm" "$tmp/got.norm" | sed 's/^/    /'
        echo "--- end diff ---"
        return 1
    fi
}

# cases
declare -a cases
while IFS= read -r d; do
    [[ -d "$d" ]] || continue
    if [[ -n "$FILTER" && "$(basename "$d")" != "$FILTER" ]]; then
        continue
    fi
    cases+=("$d")
done < <(find "$CASES_DIR" -mindepth 1 -maxdepth 1 -type d | sort)

if [[ ${#cases[@]} -eq 0 ]]; then
    echo "no test cases found${FILTER:+ matching '$FILTER'}" >&2
    exit 2
fi

# run
pass=0; fail=0; skip=0
for d in "${cases[@]}"; do
    run_case "$d"
    case $? in
        0) ((pass++)) ;;
        1) ((fail++)) ;;
        2) ((skip++)) ;;
    esac
done

echo
echo "Results: ${GREEN}${pass} passed${NC}, ${RED}${fail} failed${NC}, ${YELLOW}${skip} skipped${NC}"
[[ $fail -eq 0 ]]