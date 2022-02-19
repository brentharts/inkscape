from lxml import etree

document = etree.parse("regression-1364_output.svg")

layer = document.find('{http://www.w3.org/2000/svg}g[@id="layer1"]')
boolop_result = layer.find('{http://www.w3.org/2000/svg}path[@id="small"]')

assert boolop_result.attrib.get("transform") is None

