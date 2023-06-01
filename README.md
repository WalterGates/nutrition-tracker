# Nutrition Tracker

This is currently work in progress.

This is an application meant for helping with meal tracking.
It allows you to attach notes to individual meals, to view graphs
of your history over time, and to plan meals by their nutritional value.

# Building

You will need `git`, `cmake`, and some sort of C/C++ tool chain.

    git clone https://github.com/WalterGates/nutrition-tracker.git
    cmake . -B build
    cmake --build build -j 16

Note: This project uses `vcpkg`. If you already have it installed in the default
location, we will just use that. In case you have it installed to another location
or you don't have it at all, the cmake script will install it automatically to
the default location. 
