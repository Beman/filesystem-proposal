copy /y filesystem-proposal.html D:\filesystem-proposal\index.html
pushd D:\filesystem-proposal
call git commit -a -m "auto commit"
call git push origin gh-pages
popd
