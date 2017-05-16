#!/bin/bash

TEST_HTML_DIR="${TEST_HTML_DIR-"test-html"}"

rm -rf "${TEST_HTML_DIR}"
mkdir -p "${TEST_HTML_DIR}"

TEST_HTML_DIR="$(readlink -e "${TEST_HTML_DIR}")"

for module in ${MODULES}; do
    module="$(readlink -e "${module}")"
    moduleHtmlDir="${module}/html"
    moduleHtmlTestDir="${module}/test-html"
    pushd "${moduleHtmlDir}" > /dev/null
    find * -type d -exec mkdir -p "${TEST_HTML_DIR}/"{} \;
    for file in $(find * -type f); do
        if echo ${file} | grep '\.tpl$' > /dev/null; then
            targetFile="${TEST_HTML_DIR}/${file%%.*}.html"
        else
            targetFile="${TEST_HTML_DIR}/${file}"
        fi
        echo "${file} -> ${targetFile}"
        sedFile="${moduleHtmlTestDir}/templates/${file%%.*}.sed"
        if [ -f "${sedFile}" ]; then
            sed -f "${sedFile}" "${file}" > "${targetFile}"
        else
            cp "${file}" "${targetFile}"
        fi
    done
    test -d "${moduleHtmlTestDir}/cgi" && ls ${moduleHtmlTestDir}/cgi/ && cp -r ${moduleHtmlTestDir}/cgi/* ${TEST_HTML_DIR}/
    popd > /dev/null
done
