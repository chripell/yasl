#!/usr/bin/env python3

import setuptools

with open("README.md", "r") as fh:
    long_description = fh.read()

setuptools.setup(
    name="YASL",
    version="0.0.1",
    author="Christian Pellegrin",
    author_email="chripell@gmail.com",
    description="A small example package",
    long_description=long_description,
    long_description_content_type="text/markdown",
    url="https://github.com/chripell/yasl",
    packages=setuptools.find_packages(),
    classifiers=[
        "Programming Language :: Python :: 3",
        "License :: OSI Approved :: Apache",
        "Operating System :: OS Independent",
    ],
    python_requires='>=3.8',
)
