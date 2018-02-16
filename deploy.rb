#!/usr/bin/env ruby
# vim: set sw=4 ts=4 et :

require 'optparse'
require 'open3'
include Open3

JENKINS_URL = "https://usw1-jenkins-002.blab.im/job/gst-cef/"

def bump_version(t)
    elems = t.split(".").map{|x| x.to_i}
    elems[-1] += 1
    elems.join(".")
end

def require_clean_work_tree
    # do a git pull to make sure we are current
    system("git pull") or raise "Something went wrong with git pull"
    # update the index
    system("git update-index -q --ignore-submodules --refresh")  or raise "Something went wrong with git update-index"

    #no unstaged files allowed
    changed_files, _stderr_str, status = capture3('git status --short')
    unless status.success?
        puts "cannot check git status, exiting"
        exit(1)
    end
    unless changed_files.empty?
        puts "no unstaged files allowed. exiting"
        puts changed_files
        exit(1)
    end
end


#ARGV << '-h' if ARGV.empty?

# env, tag, deploy, hosts
# get options
options = {}
OptionParser.new do |opts|
  opts.banner = "Usage: deploy.rb [options]"

  opts.on("-v", "--[no-]verbose", "Run verbosely") do |v|
    options[:verbose] = v
  end
  opts.on("-a", "--allow-dirty", "allow dirty git repo") do |d|
    options[:dirty] = d
  end
  opts.on("-n", "--[no-]dry-run", "Dry run") do |n|
      options[:dryrun] = n
  end
  options[:environment] = "dev"
  opts.on("-e", "--env ENV", "Environment to deploy to: dev, prod, local") do |e|
      options[:environment] = e
  end
  opts.on("-t", "--tag TAG", "tag to deploy") do |t|
      options[:tag] = t
  end
  options[:deploy] = true
  opts.on("-d", "--[no-]deploy", "deploy to hosts") do |d|
      options[:deploy] = d
  end
  opts.on("-H", "--hosts HOSTS", "hosts to deploy to, comma seperated list, eg dev01,dev02 ") do |h|
      options[:hosts] = h
  end
  opts.on_tail("-h", "--help", "Show this message") do
    puts opts
    exit
  end

end.parse!

# make sure there are no uncommited changes before we tag
unless options[:dirty]
    require_clean_work_tree
end

test_response=%x(curl -L --write-out "%{http_code}" -so /dev/null #{JENKINS_URL})
# check for 200, if not, then exit
unless test_response == '200'
#if test_response != '200' || test_response != '201'
    puts "cannot contact jenkins, did you forget to connect to the VPN? #{test_response}"
    exit(1)
end

# generate tag
new_tag = ''
unless options[:tag]
    time = Time.new
    current_branch=%x(git rev-parse --abbrev-ref HEAD).chomp
    new_tag = current_branch + "-" + time.strftime("%Y%m%d%H%M%S")
#    if tag_result != '0' || tag_push_result != '0'
#        puts "tagging failed, try again later: #{tag_result}"
#    end
else
    new_tag = options[:tag]
end

puts "current branch: #{current_branch}" if options[:verbose]
puts "new tag: #{new_tag}" if options[:verbose]
unless options[:dryrun]
    system("git tag #{new_tag}") or raise "Cannot set tag: #{new_tag}"
    system("git push --tags") or raise "Cannot push tags, something went wrong"
end

# trigger new build
jenkins_build_url="#{JENKINS_URL}buildWithParameters?token=uBC3kFJF&ENV=#{options[:environment]}&TAG=#{new_tag}&DEPLOY=#{options[:deploy]}"

# append hosts parameter to jenkins url if we set it option
if options[:hosts]
    jenkins_build_url += "&HOSTS=#{options[:hosts]}"
end

puts "jenkins url: #{jenkins_build_url}" if options[:verbose]
unless options[:dryrun]
    build_response=%x(curl -sL --write-out %{http_code} '#{jenkins_build_url}')
    if build_response == '200' || build_response == '201'
        puts "building #{new_tag}"
    else
        puts "contacting jenkins failed with status code #{build_response}"
        exit(1)
    end
end

puts "You sure you want to push to `prod` and not `bebo-prod`?" if options[:environment] == "prod"
