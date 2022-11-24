@echo off

call .venv/Scripts/activate
uvicorn saltyemu:app
