#!/bin/bash
# Copyright 2025, UNSW
# SPDX-License-Identifier: BSD-2-Clause

set -e

mkdir -p ~/.ssh

cat >> ~/.ssh/known_hosts <<\EOF
login.trustworthy.systems ecdsa-sha2-nistp256 AAAAE2VjZHNhLXNoYTItbmlzdHAyNTYAAAAIbmlzdHAyNTYAAABBBD111xPT6mKt1s+wJvXIGwUXaebbM/B1GE7ztMUgKBqySbO/5AXXFUr/xflvSluH3lYG5tTpGwPYbJyHOmnJGLY=
login.trustworthy.systems ssh-rsa AAAAB3NzaC1yc2EAAAADAQABAAABgQCYZpsSbQVXO1FUK/OLYsHZm8nceROnrb/pOPoZOJjeOesE5JtL0g2c8AXWp8mEJ/8eWCUXD/iVDQs2FN9MGanh1ZgzmU6OyyE050W31owcMEX8pK8PVsHKZCal2YFKBzh5gbTKmFw7cNweXRdu49V9M8K8rc5We3TGU8NVx6c6UTOtZ/jg47t0GNukt7EKeMKlJKjxnaE32ECYhSYAmJ0u1tZQFIYP4Cz8vXcBdujdo2+7OMs5tBDhGFufITg6OQDgBFLEgRmKZCLV+17Qxop/Cp4S/BmGPpid4PfRDpTW9ccYdiuymjmoJpXY6QB+bH++gnQhxOpdfM8vKjJDxsOyUNqh0H20uIn/er6gGIdbLqS1nI/DtmGmbSfRVXBuNKxQMxcBh45MEYxrQp51p9kW/eN5/2YdFYDNSw7/fagfiwmjcKUvsiySeRXZzGu5fRHqZktKXGCFeuA6O4uXx/kqshLdGJ6X0d1Kl4v02S7Jh0pcA8i146/1he1+D4FLpb8=
login.trustworthy.systems ssh-ed25519 AAAAC3NzaC1lZDI1NTE5AAAAINn1vg1dwyMuDFVWdUqoIRmOHv9FdbCZ4q+0zDY/xTJN
login.trustworthy.systems ssh-rsa AAAAB3NzaC1yc2EAAAADAQABAAABgQDSzWm9H9EKxcinJs8qwBP3U89DBAB+aO+0zb30gynjdnTKpoCMh58LFxzWWs3bM6liNQKj2FJ7Bpog1N7qExxB3dwo34xQ1Nt4YYSE8TIgfjHGmcvo81F8mElNJYH8d7r1iFsVQcjrnhUzapMyeSIL5UtAVHcJmxKNLjgUOeP6YrAE8q6e0Ods1dLS0p3K4IA1LMz57gTdeZfld3GJ/LPg/6D7LEcxkPX+KG15Y7J/zc1uE0GuPFfLYY16rCcbY0ezhqqqgCig/PIQPqc12g6m+n2WkSjMDv6XycyedoCORKySfOCCBkmI8BkTmoO7Nlm/tQ8UbGDH+6o5CiP5WjEDvbf6Gm9gBANemfxP6Nkyhf+0HrBTXSKjSYVXe99q6YZOG2TvUiDsR8Y7cdWDukbge2AdSy7aRRGPnKPWk2HkFBXahpHQm/wFft/1DZaurX/Vee2GlsFXCGkog0vWi2COvWcGFTIMPwcSyuoLTc5exoy5zF3tcDsPWagkLSxBxbU=
login.trustworthy.systems ecdsa-sha2-nistp256 AAAAE2VjZHNhLXNoYTItbmlzdHAyNTYAAAAIbmlzdHAyNTYAAABBBCRA/W8LVxLFjzPvijygdSw+rPW/EQEG8WoUVcTm5dYXDIhCc0Zxibd19zPb1LQpE2/Ohe+I16iC5glpmFyDfrs=
login.trustworthy.systems ssh-ed25519 AAAAC3NzaC1lZDI1NTE5AAAAIPYp/3vDMDnHnjtqt5Oqievgz04g/LJ4yEKOlXCu9Yux
tftp.keg.cse.unsw.edu.au ecdsa-sha2-nistp256 AAAAE2VjZHNhLXNoYTItbmlzdHAyNTYAAAAIbmlzdHAyNTYAAABBBEj7X6doSoop91gTvBD7L4O7VGwCO5pLNsu5YAGS1L64MJqo+3wTYgFRdMWTM0hL3YN+1sSabJPICJzKk0EJxkg=
EOF

cat >> ~/.ssh/config <<EOF
Host *
  ServerAliveInterval 30
  ControlMaster auto
  ControlPath ~/.ssh/%r@%h:%p-${GITHUB_RUN_ID}-${GITHUB_JOB}-${INPUT_INDEX}
  ControlPersist 15

Host ts
  Hostname login.trustworthy.systems
  User ts_ci

Host tftp.keg.cse.unsw.edu.au
  User ts_ci
  ProxyJump ts

EOF

echo "${MACHINE_QUEUE_KEY}" > ~/.ssh/id_ed25519
chmod 600 ~/.ssh/id_ed25519
