# GitHub publication checklist

Owner: `Active-Software`

Suggested repository name: `iperf3-windows-msvc`

Visibility: public

Attribution: Daniel Doguet / Active-Software

## Create the repository

1. Open GitHub in the browser.
2. Switch owner to `Active-Software`.
3. Create a new public repository named `iperf3-windows-msvc`.
4. Do not add a README, license, or gitignore in GitHub. This folder already has them.

## Upload from GitHub Desktop

If GitHub Desktop is installed:

1. File > Add local repository.
2. Select `C:\Sources\iperf3`.
3. If prompted, choose "create a repository" from this folder.
4. Publish repository.
5. Choose owner `Active-Software`.
6. Keep it public.

## Upload from command line

If `git` is available:

```powershell
cd C:\Sources\iperf3
git init
git add .
git commit -m "Add native Windows MSVC build for iperf3 3.21"
git branch -M main
git remote add origin https://github.com/Active-Software/iperf3-windows-msvc.git
git push -u origin main
```

## Create a release

1. Open the repository on GitHub.
2. Go to Releases > Draft a new release.
3. Tag: `v3.21-windows-msvc-1`
4. Title: `iperf 3.21 native Windows MSVC build`
5. Paste the content of `GITHUB_RELEASE_v3.21-windows-msvc-1.md`.
6. Upload the files from `dist\`.
7. Publish release.
