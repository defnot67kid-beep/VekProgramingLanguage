# Push VEK 1.5.0 to GitHub

For the replace-folder workflow: extract this ZIP, rename the extracted folder to `VEK-Language`, open Command Prompt in it, then run:

```bat
git init
git remote add origin https://github.com/defnot67kid-beep/VekProgramingLanguage.git
git fetch origin
git reset origin/main
git branch -M main
git add -A
git commit -m "VEK 1.5 garage passlocks and advanced access UI"
git push -u origin main
git tag v1.5.0
git push origin v1.5.0
```

Do not use `git reset --hard origin/main` after extracting the new files; that would replace the new version with the older remote contents.
