# Push VEK 1.4.0 to GitHub

Repository:
`https://github.com/defnot67kid-beep/VekProgramingLanguage.git`

After replacing your local repository files with this VEK 1.4 package:

```bat
git add -A
git commit -m "VEK 1.4 animation and proximity prompt systems"
git push origin main
git tag v1.4.0
git push origin v1.4.0
```

If `v1.4.0` already exists locally and points at the wrong commit, delete/recreate it before pushing. Do not force-update a published tag unless you intentionally want to replace it.
