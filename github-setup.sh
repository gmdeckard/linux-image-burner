#!/bin/bash
# GitHub Setup Script for linux-image-burner
# Run this AFTER creating the GitHub repository

echo "Setting up GitHub remote for linux-image-burner..."

# Add GitHub remote
echo "Adding GitHub remote..."
git remote add origin https://github.com/gmdeckard/linux-image-burner.git

# Rename branch to main
echo "Renaming branch to main..."
git branch -M main

# Push to GitHub
echo "Pushing to GitHub..."
git push -u origin main

echo "✅ Successfully pushed to GitHub!"
echo "Your repository is now available at: https://github.com/gmdeckard/linux-image-burner"
