@echo off
rem Commits and pushes everything using the message in .commitmsg.
cd /d "%~dp0"
if not exist .git (
  git init -b main
  git lfs install
  git remote add origin https://github.com/reixyz22/Unreal_Engine_game.git
)
git add -A
git status --short
git commit -F .commitmsg
git push -u origin main
pause
