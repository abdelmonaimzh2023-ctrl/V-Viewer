#!/bin/sh
case "`uname`" in
  CYGWIN*|MINGW*|MSYS*) cygwin=true ;;
esac
DIRNAME=`dirname "$0"`
APP_HOME=`cd "$DIRNAME" ; pwd -P`
CLASSPATH=$APP_HOME/gradle/wrapper/gradle-wrapper.jar
exec java -Xmx2048m -classpath "$CLASSPATH" org.gradle.wrapper.GradleWrapperMain "$@"
