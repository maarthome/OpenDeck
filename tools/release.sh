#!/bin/bash

#read MAJOR file
major=`cat MAJOR_NEW`

#read MINOR file
minor=`cat MINOR_NEW`

#read REVISION file
revision=`cat REVISION`

git tag v$major.$minor.$revision
git push --tags