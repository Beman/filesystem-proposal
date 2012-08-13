copy /y filesystem-proposal.html D:\filesystem-proposal\index.html
pushd D:\filesystem-proposal-mods
git commit -a -m "auto commit"
git push origin gh-pages
popd

