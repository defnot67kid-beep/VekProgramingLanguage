# Update your VEK GitHub repository to 1.2.0

Repository:
`https://github.com/defnot67kid-beep/VekProgramingLanguage.git`

Replace/update your local repository files using this VEK 1.2 ZIP, then run:

```bash
git add .
git commit -m "VEK 1.2 vehicle editor and game mode systems"
git push origin main
```

Create the exact tag used by game v12:

```bash
git tag v1.2.0
git push origin v1.2.0
```

If the tag already exists and points to an older commit, do not silently overwrite a public release tag. Create a new version such as `v1.2.1` and change the game's `VEK_GITHUB_TAG.txt` to match.
