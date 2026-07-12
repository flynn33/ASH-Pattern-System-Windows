#!/bin/bash
cd /d/GitHub/APS
git add windows/src/*.cpp
git add windows/tests/*.cpp  
git add docs/**/*.md
git add completion-evidence/**/*.md
git add completion-evidence/**/*.json
git commit -m "Add ASH Pattern System source code and documentation

Core implementation: StateModel, RecoveryEngine, TransitionRegistry,
Diagnostics, RealmEncoder, TopologyGenerator, AxiomEvaluator, 
GenerationPlanner, ArtifactEmitter"
