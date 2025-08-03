#!/usr/bin/env pwsh

# Script to automate fixing git commit messages
# Usage: ./fix-commits.ps1

# Dictionary of commits to fix with their new messages
$commits = @{
    "cb2b41f" = "Updated editor icon with new design";
    "9bd9fef" = "Fixed icon display issues";
    "cc10e21" = "Fixed UI styling issues";
    "51183eb" = "Updated map rendering functionality";
    "610eaa9" = "Added boundary box improvements";
    "72faa74" = "Fixed texture loading bug";
    "460d07b" = "Improved sprite rendering";
    "eec2b88" = "Updated keyboard shortcuts";
    "b8f5a54" = "Fixed display offset issue";
}

# Check for unstaged changes
$hasChanges = $(git status --porcelain) -ne $null
if ($hasChanges) {
    Write-Host "You have unstaged changes. Please commit or stash them before running this script." -ForegroundColor Red
    Write-Host "Run 'git status' to see your changes."
    exit 1
}

# Save the current branch name
$currentBranch = git rev-parse --abbrev-ref HEAD

# Check if temp branch already exists and delete it if it does
$branchExists = git branch --list "temp-fix-commits"
if ($branchExists) {
    Write-Host "Removing existing temp-fix-commits branch..." -ForegroundColor Yellow
    git branch -D temp-fix-commits
}

# Suppress git filter-branch warnings
$env:FILTER_BRANCH_SQUELCH_WARNING = 1

# Create a temporary branch for our work
git checkout -b temp-fix-commits

# Process each commit - stop on errors
$ErrorActionPreference = "Stop"
foreach ($commit in $commits.GetEnumerator() | Sort-Object -Property Name) {
    Write-Host "Fixing commit: $($commit.Key) -> $($commit.Value)" -ForegroundColor Cyan
    
    # Check if commit exists
    $commitExists = git rev-parse --verify --quiet "$($commit.Key)^{commit}"
    if (-not $commitExists) {
        Write-Host "Commit $($commit.Key) not found, skipping..." -ForegroundColor Yellow
        continue
    }
    
    # Use filter-branch to rewrite the commit message
    git filter-branch --force --msg-filter "if [ `"`$(git rev-parse --short HEAD)`" = '$($commit.Key)' ]; then echo '$($commit.Value)'; else cat; fi" $($commit.Key)^..HEAD
}

# Force update the original branch
Write-Host "Updating original branch: $currentBranch" -ForegroundColor Green
git checkout $currentBranch
git reset --hard temp-fix-commits

# Clean up
git branch -D temp-fix-commits
git reflog expire --expire=now --all
git gc --prune=now

Write-Host "Done! Commit messages have been updated." -ForegroundColor Green 