# Copyright 2013-2019 Lawrence Livermore National Security, LLC and other
# Spack Project Developers. See the top-level COPYRIGHT file for details.
#
# SPDX-License-Identifier: (Apache-2.0 OR MIT)


from spack import *


class Flecsi(CMakePackage):
    '''FleCSI is a compile-time configurable framework designed to support
       multi-physics application development. As such, FleCSI attempts to
       provide a very general set of infrastructure design patterns that can
       be specialized and extended to suit the needs of a broad variety of
       solver and data requirements. Current support includes multi-dimensional
       mesh topology, mesh geometry, and mesh adjacency information,
       n-dimensional hashed-tree data structures, graph partitioning
       interfaces,and dependency closures.
    '''
    homepage = 'http://flecsi.org/'
    git      = 'https://github.com/laristra/flecsi.git'

    version('master', branch='master', submodules=False, preferred=True)

    variant('build_type', default='Release', values=('Debug', 'Release'),
            description='The build type to build', multi=False)
    variant('backend', default='mpi', values=('serial', 'mpi', 'legion', 'hpx'),
            description='Backend to use for distributed memory', multi=False)
    variant('debug_backend', default=False,
            description='Build Backend with Debug Mode')
    variant('minimal', default=False,
            description='Disable FindPackageMetis')
    variant('shared', default=True,
            description='Build shared libraries')
    variant('flog', default=False,
            description='Enable flog testing')
    variant('doxygen', default=False,
            description='Enable doxygen')
    variant('doc', default=False,
            description='Enable documentation')
    variant('coverage', default=False,
            description='Enable coverage build')
    variant('hdf5', default=False,
            description='Enable HDF5 Support')
    variant('caliper', default=False,
            description='Enable Caliper Support')
    variant('graphviz', default=False,
            description='Enable GraphViz Support')
    variant('tutorial', default=False,
            description='Build FleCSI Tutorials')
    variant('flecstan', default=False,
            description='Build FleCSI Static Analyzer')

    depends_on('cmake@3.12:',  type='build')
    # Requires cinch > 1.0 due to cinchlog installation issue
    #depends_on('cinch@1.01:', type='build')
    depends_on('mpi', when='backend=mpi')
    depends_on('mpi', when='backend=legion')
    depends_on('mpi', when='backend=hpx')
    depends_on('legion@ctrl-rep +shared +mpi build_type=Debug +hdf5', when='backend=legion +debug_backend +hdf5')
    depends_on('legion@ctrl-rep +shared +mpi build_type=Debug', when='backend=legion +debug_backend ~hdf5')
    depends_on('legion@ctrl-rep +shared +mpi +hdf5', when='backend=legion +hdf5')
    depends_on('legion@ctrl-rep +shared +mpi', when='backend=legion ~hdf5')
    depends_on('hpx@1.3.0 cxxstd=14 build_type=Debug', when='backend=hpx +debug_backend')
    depends_on('hpx@1.3.0 cxxstd=14', when='backend=hpx')
    depends_on('boost@1.70.0: cxxstd=14 +program_options')
    depends_on('metis@5.1.0:')
    depends_on('parmetis@4.0.3:')
    depends_on('hdf5', when='+hdf5')
    depends_on('caliper', when='+caliper')
    depends_on('graphviz', when='+graphviz')
    depends_on('python@3.0:', when='+tutorial')
    depends_on('llvm', when='+flecstan')

    conflicts('+tutorial', when='backend=hpx')
#    conflicts('+hdf5', when='backend=hpx')
#    conflicts('+hdf5', when='backend=mpi')

    def cmake_args(self):
        spec = self.spec
        options = ['-DENABLE_OPENMP=ON',
                   '-DCXX_CONFORMANCE_STANDARD=c++17',
                   '-DENABLE_METIS=ON',
                   '-DENABLE_PARMETIS=ON',
                   '-DENABLE_COLORING=ON',
                   '-DENABLE_DEVEL_TARGETS=ON'
                   ]
        #options.append('-DCINCH_SOURCE_DIR=' + spec['cinch'].prefix)

        if spec.variants['backend'].value == 'legion':
            options.append('-DFLECSI_RUNTIME_MODEL=legion')
            options.append('-DENABLE_MPI=ON')
        elif spec.variants['backend'].value == 'mpi':
            options.append('-DFLECSI_RUNTIME_MODEL=mpi')
            options.append('-DENABLE_MPI=ON')
        elif spec.variants['backend'].value == 'hpx':
            options.append('-DFLECSI_RUNTIME_MODEL=hpx')
            options.append('-DENABLE_MPI=ON')
        else:
            options.append('-DFLECSI_RUNTIME_MODEL=serial')
            options.append('-DENABLE_MPI=OFF')

        if self.run_tests:
            options.append('-DENABLE_UNIT_TESTS=ON')
        else:
            options.append('-DENABLE_UNIT_TESTS=OFF')

        if '+minimal' in spec:
            options.append('-DCMAKE_DISABLE_FIND_PACKAGE_METIS=ON')
        else:
            options.append('-DCMAKE_DISABLE_FIND_PACKAGE_METIS=OFF')
        if '+shared' in spec:
            options.append('-DBUILD_SHARED_LIBS=ON')
        else:
            options.append('-DBUILD_SHARED_LIBS=OFF')

        if '+hdf5' in spec and spec.variants['backend'].value == 'legion':
            options.append('-DENABLE_HDF5=ON')
        else:
            options.append('-DENABLE_HDF5=OFF')
        if '+caliper' in spec:
            options.append('-DENABLE_CALIPER=ON')
        else:
            options.append('-DENABLE_CALIPER=OFF')

        if '+tutorial' in spec:
            options.append('-DENABLE_FLECSIT=ON')
            options.append('-DENABLE_FLECSI_TUTORIAL=ON')
        else:
            options.append('-DENABLE_FLECSIT=OFF')
            options.append('-DENABLE_FLECSI_TUTORIAL=OFF')
        if '+flecstan' in spec:
            options.append('-DENABLE_FLECSTAN=ON')
        else:
            options.append('-DENABLE_FLECSTAN=OFF')

        if '+doxygen' in spec:
            options.append('-DENABLE_DOXYGEN=ON')
        else:
            options.append('-DENABLE_DOXYGEN=OFF')
        if '+doc' in spec:
            options.append('-DENABLE_DOCUMENTATION=ON')
        else:
            options.append('-DENABLE_DOCUMENTATION=OFF')
        if '+coverage' in spec:
            options.append('-DENABLE_COVERAGE_BUILD=ON')
        else:
            options.append('-DENABLE_COVERAGE_BUILD=OFF')

        return options
