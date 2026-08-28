# Push VEK to GitHub

After creating an empty GitHub repository, open a terminal in this folder:

```bash
git init
git add .
git commit -m "VEK 1.0 initial release"
git branch -M main
git remote add origin https://github.com/YOUR_NAME/YOUR_REPO.git
git push -u origin main
```

Then place the repository URL into the game's `VEK_GITHUB_REPO.txt` and rebuild. The game uses CMake FetchContent to download VEK.
