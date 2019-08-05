# Mako Authentication

The Mako command-line tool and the Mako clients communicate with
https://mako.dev using the Application Default Credentials (ADC) strategy. You
can find full documentation of this strategy at
http://cloud/docs/authentication/production.

When running locally and wanting to authenticate as yourself (as opposed to as a
service account), the easiest way to set up authentication is using the gcloud
CLI, which is part of the Google Cloud SDK. Instructions for installing the SDK
and CLI can be found here: https://cloud.google.com/sdk/gcloud/.

Once the SDK is installed, execute the following command to establish
credentials that the Mako tools/clients can use:

```bash
gcloud auth application-default login
```

Follow the instructions, which will involve visiting a web page, copying an
access token, and pasting it in the command-line prompt. Documentation for this
command can be found here:
https://cloud.google.com/sdk/gcloud/reference/auth/application-default/login.
