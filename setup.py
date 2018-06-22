#!/usr/bin/env python3

import sys
from setuptools import setup
from setuptools import find_packages

setup(
    name="flipsyfat",
    version="0.1.dev",
    description="FPGA-based emulator to assist with guessing bootloader SD card filenames",
    long_description=open("README.md").read(),
    author="Micah Elizabeth Scott",
    author_email="micah@misc.name",
    url="http://scanlime.org",
    download_url="https://github.com/scanlime/flipsyfat",
    license="MIT",
    platforms=["Any"],
    keywords="HDL ASIC FPGA hardware design",
    classifiers=[
        "Topic :: Scientific/Engineering :: Electronic Design Automation (EDA)",
        "Environment :: Console",
        "Development Status :: Alpha",
        "Intended Audience :: Developers",
        "License :: OSI Approved :: MIT License",
        "Operating System :: OS Independent",
        "Programming Language :: Python",
    ],
    packages=find_packages(),
    install_requires=["misoc"],
    package_data={'': ['verilog/*']},
    include_package_data=True,
)
