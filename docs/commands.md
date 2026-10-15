(git diff HEAD; git ls-files --others --exclude-standard | while IFS= read -r file; do echo; echo "===== NEW FILE: $file ====="; cat "$file"; done) > changes.txt

find README.md Makefile linker.ld src include docs bugs -type f \( -name "*.c" -o -name "*.h" -o -name "*.asm" -o -name "*.md" -o -name "*.ld" -o -name "Makefile" \) -exec tail -n +1 {} + > codebase.txt