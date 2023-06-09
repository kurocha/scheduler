# Teapot v3.5.2 configuration generated at 2023-04-19 19:39:36 +1200

required_version "3.0"

define_project "scheduler" do |project|
	project.title = "Scheduler"
end

# Build Targets

define_target 'scheduler-library' do |target|
	target.depends 'Language/C++17'
	target.depends 'Build/Compile/Commands'
	
	target.depends "Library/Time", public: true
	target.depends "Library/Concurrent", public: true
	
	target.provides 'Library/Scheduler' do
		source_root = target.package.path + 'source'
		
		library_path = build static_library: 'Scheduler', source_files: source_root.glob('Scheduler/**/*.cpp')
		
		append linkflags library_path
		append header_search_paths source_root
		
		compile_commands destination_path: (source_root + "compile_commands.json")
	end
end

define_target 'scheduler-test' do |target|
	target.depends 'Library/Scheduler'
	target.depends 'Library/UnitTest'
	
	target.depends 'Language/C++17'
	target.depends 'Build/Compile/Commands'
	
	target.provides 'Test/Scheduler' do |arguments|
		test_root = target.package.path + 'test'
		
		run tests: 'Scheduler-tests', source_files: test_root.glob('Scheduler/**/*.cpp'), arguments: arguments
		
		compile_commands destination_path: (test_root + "compile_commands.json")
	end
end

# Configurations

define_configuration 'development' do |configuration|
	configuration[:source] = "https://github.com/kurocha"
	configuration.import "scheduler"
	
	# Provides all the build related infrastructure:
	configuration.require 'platforms'
	
	# Provides unit testing infrastructure and generators:
	configuration.require 'unit-test'
	
	# Provides some useful C++ generators:
	configuration.require 'generate-cpp-class'
	
	configuration.require "generate-project"
	
	configuration.require "build-compile-commands"
end

define_configuration "scheduler" do |configuration|
	configuration.public!
	
	configuration.require "concurrent"
	configuration.require "time"
end
