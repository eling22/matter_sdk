import os
import json
import requests
import sys

# Authentication for user filing issue (must have read/write access to
# repository to add issue to)
USERNAME = 'billkarma-bot'
TOKEN = os.environ['GITHUB_TOKEN']

# The repository to add this issue to
REPO_OWNER = 'footbits'
REPO_NAME = 'billkarma-ios'

print("os.environ :", os.environ)

# def make_github_issue(title, body=None, labels=None):
#     '''Create an issue on github.com using the given parameters.'''
#     # Our url to create issues via POST
#     url = 'https://api.github.com/repos/%s/%s/issues' % (REPO_OWNER, REPO_NAME)
#     # Create an authenticated session to create the issue
#     headers = {'Authorization': "token %s" % TOKEN,
#                "Accept": "application/vnd.github+json" }
#     # Create our issue
#     issue = {'title': title,
#              'body': body,
#              'labels': labels,
#              'assignees': 'eling22'}
#     # Add the issue to our repository
#     r = requests.request('POST', url, headers=headers, data=json.dumps(issue))
#     if r.status_code == 201:
#         print('Successfully created Issue "%s"' % title)
#     else:
#         print('Could not create Issue "%s"' % title)
#         print('Response:', r.content)

# make_github_issue('Issue Title', 'Body text', ['bug'])