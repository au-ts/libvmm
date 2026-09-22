#
# Copyright 2026, UNSW
# SPDX-License-Identifier: BSD-2-Clause
#
{
  stdenv,
  lib,
  elfutils,
  bc,
  bison,
  dtc,
  fetchFromGitHub,
  fetchpatch,
  fetchurl,
  flex,
  perl,
  gnutls,
  installShellFiles,
  libuuid,
  meson-tools,
  ncurses,
  openssl,
  python3,
  pkg-config,
  which,
  buildPackages,
  callPackages,
}@pkgs:

  lib.makeOverridable (
    {
      arch,
      version,
      src,
      installDir ? "$out",
      configFile,
      extraPatches ? [ ],
      extraMakeFlags ? [ ],
      stdenv ? pkgs.stdenv,
      # TODO: actually implement
      extraFilesToInstall ? [ ],
      ...
    }@args:
    stdenv.mkDerivation (
      {
        inherit version src;

        pname = "linux-${arch}-${configFile}";

        patches = extraPatches;

        # postPatch = ''
        #   ${lib.concatMapStrings (script: ''
        #     substituteInPlace ${script} \
        #     --replace "#!/usr/bin/env python3" "#!${pythonScriptsToInstall.${script}}/bin/python3"
        #   '') (builtins.attrNames pythonScriptsToInstall)}
        #   patchShebangs tools
        #   patchShebangs scripts
        # '';

        nativeBuildInputs = [
          ncurses # tools/kwboot
          perl
          bc
          bison
          openssl
          pkg-config
          flex
          installShellFiles
          python3
          which
          elfutils
        ];
        depsBuildBuild = [ buildPackages.gccStdenv.cc ]; # gccStdenv is needed for Darwin buildPlatform

        hardeningDisable = [ "all" ];

        enableParallelBuilding = true;

        makeFlags = [
          "ARCH=${arch}"
          "DTC=${lib.getExe buildPackages.dtc}"
          "CROSS_COMPILE=${stdenv.cc.targetPrefix}"
          "HOSTCFLAGS=-fcommon"
        ]
        ++ extraMakeFlags;

        passAsFile = [ "extraConfig" ];

        configurePhase = ''
          runHook preConfigure

          cp ${configFile} .config
          make $makeFlags -j$NIX_BUILD_CORES

          runHook postConfigure
        '';

        installPhase = ''
          runHook preInstall

          mkdir -p ${installDir}
          cp .config ${installDir}/config
          cp vmlinux ${installDir}
          ls -l arch/${arch}/boot/Image && cp arch/${arch}/boot/Image ${installDir}
          ls -l arch/${arch}/boot/Image.gz && cp arch/${arch}/boot/Image.gz ${installDir}
          ls -l arch/${arch}/boot/bzImage && cp arch/${arch}/boot/bzImage ${installDir}

          runHook postInstall
        '';

        dontStrip = true;

      }
    )
  )
