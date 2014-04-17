#!/usr/bin/python

"""
__version__ = "$Revision: 1.1 $"
__date__ = "$Date: 2004/09/29 19:04:09 $"
"""

import os
import shutil
import re
import string
import copy

# this routine copied from O'reilly book "Python Standard Library", p.27
def RunProgram( program, *args ):
  # find executable
  for path in string.split( os.environ['PATH'], os.pathsep ):
    filePath = os.path.join( path, program ) + ".exe"
    try:
      return os.spawnv( os.P_WAIT, filePath, (program,) + args )
    except os.error:
      pass
  raise os.error, "cannot find executable"

def RunCommand( commandText ):
  (inputFile,outputFile,errorFile) = os.popen3( commandText )
  inputFile.close()
  outputText = outputFile.read()     # Get child's output
  outputFile.close()
  errorText = errorFile.read()     # Get child's errors
  errorFile.close()
  return (outputText,errorText)

def dconnect( server ):
  return RunCommand( "dconnect " + server + " 130.126.127" )
  
def dlogin( user ):
  return RunCommand( "dlogin " + user )
  
def dps():
  return RunCommand( "dps" )

def killalldemo( cluster ):
  return RunCommand( "killalldemo " + cluster )
  
def dbatch( filePath ):
  return RunCommand( "dbatch " + filePath )

def dset( computerName, groupName, parameterName, value ):
  return RunCommand( "dset " + computerName + " " + groupName + " " \
            + parameterName + " " + value )
            
def dget( computerName, groupName, parameterName ):
  return RunCommand( "dget " + computerName + " " + groupName + " " + parameterName )

def dex( computerName, appName ):
  return RunCommand( "dex " + computerName + " " + appName )
  
def ReadFile( filePath ):
  try:
    inputFile = file( filePath, "r" )
  except IOError:
    print "Error: Failed to open file ", filePath
    return ""
  try:
    inputString = inputFile.read()
  except IOError:
    print "Error: Failed to read file", filePath
    inputFile.close()
    return ""
  inputFile.close()
  return inputString

def WriteFile( filePath, contents ):
  try:
    outputFile = file( filePath, "w" )
  except IOError:
    print "Error: Failed to open file ", filePath
    return False
  try:
    outputFile.write( contents )
  except IOError:
    print "Error: Failed to write file ", filePath
    outputFile.close()
    return False
  outputFile.close()
  return True

def ExtractXMLTokens( theString, tag ):
  regString = "<" + tag + ">\s*"
  regex = re.compile( regString )
  matchList = regex.finditer( theString )
  startList = []
  for match in matchList:
    startList.append( match.end() )
  regString = "\s*</" + tag + ">"
  regex = re.compile( regString )
  matchList = regex.finditer( theString )
  endList = []
  for match in matchList:
    endList.append( match.start() )
  if not len( startList ) == len( endList ):
    return []
  tokenList = []
  for i in range(len(startList)):
    tokenString = theString[startList[i]:endList[i]]
    tempList = tokenString.split("\n")
    tempList = [string.strip(item) for item in tempList]
    tokenString = string.join( tempList, " " )
    tokenList.append( tokenString )
  return tokenList

def ExtractXMLToken( theString, tag ):
  temp = ExtractXMLTokens( theString, tag )
  if not len(temp) == 1:
    print "ExtractXMLToken Error: more than one <" + tag + "> tag found in ", theString
    print theString
    return None
  return temp[0]

def ParseXMLHeader( inputString, recordName, fieldNames ):
  headerList = ExtractXMLTokens( inputString, recordName )
  if len(headerList) > 1:
    print "ParseXMLHeader error: more than one header found."
    return {}
  header = headerList[0]
  headerDict = {}
  for field in fieldNames:
    valueList = ExtractXMLTokens( header, field )
    if len(valueList) > 1:
      print "ParseXMLHeader error: more than one '" + field + "' found."
      return {}
    value = valueList[0]
    fieldList = value.split("|")
    if len(fieldList) == 1:
      headerDict[field] = fieldList[0]
    else:
      headerDict[field] = fieldList
  return headerDict

def ParseNamedXMLRecords( inputString, recordName, fieldNames ):
  recordStringList = ExtractXMLTokens( inputString, recordName )
  recordList = []
  for record in recordStringList:
    recordDict = {}
    for field in fieldNames:
      valueList = ExtractXMLTokens( record, field )
      if len(valueList) > 1:
        print "ParseNamedXMLRecords error: more than one '" + field + "' found."
        return {}
      recordDict[field] = valueList[0]
    recordList.append( recordDict )
  return recordList

def ParseXMLDataFile( inputString ):
  exptDescription = ParseXMLHeader( inputString, "experiment_description", ["experiment_name","runtime","comment","factor_names","factor_types","data_names","data_types"] )
  subjectDescription = ParseXMLHeader( inputString, "subject_record_description", ["field_names","field_types"] )
  subjectFieldList = subjectDescription['field_names']
  subjectRecordList = ParseNamedXMLRecords( inputString, "subject_record", subjectFieldList )
  if len(subjectRecordList) > 1:
    print "ParseXMLDataFile error: found more than one subject record."
    return {}
  subjectRecordDict = subjectRecordList[0]
  trialDescriptionList = exptDescription['factor_names']
  trialDescriptionList.extend(exptDescription['data_names'])
  trialRecordList = ParseNamedXMLRecords( inputString, "trial_data", trialDescriptionList )
  headerDict = {}
  headerDict['experiment_name'] = exptDescription['experiment_name']
  headerDict['comment'] = exptDescription['comment']
  headerDict['runtime'] = exptDescription['runtime']
  headerDict['data_names'] = trialDescriptionList
  exptDict = {}
  exptDict['header'] = headerDict
  exptDict['subject'] = subjectRecordDict
  exptDict['trial_data'] = trialRecordList
  return exptDict

def ParseXMLConfigFile( inputString ):
  exptDescription = ParseXMLHeader( inputString, "experiment_description", ["experiment_name","method","subject","num_trials","comment","factor_names","factor_types"] )
  trialDescriptionList = exptDescription['factor_names']
  trialRecordList = ParseNamedXMLRecords( inputString, "trial_record", trialDescriptionList )
  exptDict = {}
  exptDict['header'] = exptDescription
  exptDict['trial_records'] = trialRecordList
  return exptDict

def ListToTabDelimited( inputList ):
  return string.join([string.join(item,"\t") for item in inputList],"\n")
  
def TabDelimitedToList( inputString ):
  return [item.split("\t") for item in inputString.split("\n")]
  
def ExptDictToList( exptDict, headerFields, subjectFieldsIn, \
     scalarFields, objectFields, commaFields, includeHeader=True ):
  subjectFields = copy.deepcopy( subjectFieldsIn )
  headerDict = exptDict['header']
  subjectDict = exptDict['subject']
  trialList = exptDict['trial_data']
  fileList = []
  if includeHeader:
    for field in headerFields:
      fileList.append( [field, headerDict[field]] )
    fileList.append( ['Subject Data:'] )
    for field in subjectFields:
      fileList.append( [field, subjectDict[field]] )
    fileList.append( [] )
  subjectFields.remove("name")
  allFields = []
  allFields = subjectFields[:]
  allFields.extend( scalarFields )
  for field in objectFields:
    for i in range(1,9):
      fieldNum = field + "_" + str(i)
      if field.find("positions") == -1:
        allFields.append( fieldNum )
      else:
        allFields.extend( [fieldNum +"_x", fieldNum + "_y", fieldNum + "_z"] )
  allFields.extend( commaFields )
  fileList.append( allFields )
  numTrials = len(trialList)
  for trialDict in trialList:
    tempList = [subjectDict[field] for field in subjectFields]
    tempList.extend( [trialDict[field] for field in scalarFields] )
    for field in objectFields:
      fieldNumbers = trialDict[field].split(" ")
      tempList.extend( fieldNumbers )
      if field.find("positions") == -1:
        fieldWidth = 8
      else:
        fieldWidth = 24
      for i in range(fieldWidth-len(fieldNumbers)):
        tempList.append("")
    tempList.extend( [string.join(trialDict[field].split(" "),",") for field in commaFields] )
    fileList.append( tempList )
  return fileList
        
def ParseSubjectDBHeader( dbString ):
  header = ExtractXMLTokens( dbString, "subject_record_description" )[0]
  fieldNames = ExtractXMLTokens( header, "field_names" )[0]
  fieldList = fieldNames.split("|")
  return fieldList

def ParseSubjectDatabase( dbString ):
  fieldNames = ParseSubjectDBHeader( dbString )
  recordList = ExtractXMLTokens( dbString, "subject_record" )
  dictList = []
  for item in recordList:
    theDict = {}
    for field in fieldNames:
      theDict[field] = ExtractXMLTokens( item, field )[0]
    dictList.append( theDict )
  return dictList

def ParseDataFileHeader( dataString ):
  headerString = ExtractXMLTokens( dataString, "experiment_description" )[0]
  headerDict = {}
  headerDict['experiment_name'] = ExtractXMLTokens( headerString, "experiment_name" )[0]
  headerDict['runtime'] = ExtractXMLTokens( headerString, "runtime" )[0]
  headerDict['comment'] = ExtractXMLTokens( headerString, "comment" )[0]
  headerDict['factor_names'] = ExtractXMLTokens( headerString, "factor_names" )[0].split("|")
  headerDict['data_names'] = ExtractXMLTokens( headerString, "data_names" )[0].split("|")
  return headerDict

def ParseDataFile( dataString ):
  headerDict = ParseDataFileHeader( dataString )
  subjectList = ParseSubjectDatabase( dataString )[0]
  trialStringList = ExtractXMLTokens( dataString, "trial_data" )
  factorNameList = headerDict['factor_names']
  dataNameList = headerDict['data_names']
  trialDictList = []
  for trialString in trialStringList:
    tempDict = {}
    for factorName in factorNameList:
      tempDict[factorName] = ExtractXMLTokens( trialString, factorName )[0]
    for dataName in dataNameList:
      tempDict[dataName] = ExtractXMLTokens( trialString, dataName )[0]
    trialDictList.append( tempDict )
  fileDict = {}
  fileDict['header'] = headerDict
  fileDict['subject'] = subjectList
  fileDict['trial_data'] = trialDictList
  return fileDict

def MakeXMLHeader( headerDict ):
  formatString = "<experiment_description>\n  <experiment_name>%s</experiment_name>\n  <method>enumerated</method>\n  <subject>%s</subject>\n  <num_trials>%u</num_trials>\n  <comment>%s</comment>\n  <factor_names>%s</factor_names>\n  <factor_types>%s</factor_types>\n</experiment_description>\n\n"
  return formatString % ( headerDict['experiment_name'], headerDict['subject'], int(headerDict['num_trials']), headerDict['comment'], string.join( headerDict['factor_names'], "|" ), string.join( headerDict['factor_types'], "|" ) )

def MakeXMLTrialRecords( trialList ):
  s = ""
  for trial in trialList:
    tags = trial.keys()
    s += '<trial_record>\n'
    for tag in tags:
      s += '  <%s>%s</%s>\n' % (tag,trial[tag],tag)
    s += '</trial_record>\n\n'
  return s
  
def MakeXMLConfigFile( exptConfigDict ):
  return MakeXMLHeader( exptConfigDict['header'] ) + MakeXMLTrialRecords( exptConfigDict['trial_records'] )
  
def ParseSubjectInfoString( subjectInfoString ):
  subjectStringList = subjectInfoString.split("\n")
  subjectList = []
  for item in subjectStringList:
    if len(item) > 0:
      try:
        value = eval( item )
      except StandardError, arg:
        print "The following error: '", arg, ".' occurred while evaluating record (", item ,")"
      else:
        subjectList.append( value )
  return subjectList
  
def MakeSubjectInfoString( subjectDictList ):
  goodList = []
  for item in subjectDictList:
    if not item.has_key("name"):
      print "MakeSubjectInfoString error: record(s) with no <name> field"
      print item
    else:
      goodList.append( item )
  return string.join( [repr(item) for item in goodList], "\n" )
    
def MakeExptPath( exptPath, exptName ):
  return os.path.join( os.path.normpath( exptPath ), exptName+"Data" )
  #return exptPath + "\\" + exptName + "Data"

def MakeSubjectPath( exptPath, exptName, subject ):
  return os.path.join( MakeExptPath( exptPath, exptName ), subject )
  #return MakeExptPath( exptPath, exptName ) + "\\" + subject

def GetValidConfigFiles( subjectDirectory ):
  directoryList = os.listdir( subjectDirectory )
  directoryList = [item for item in directoryList if not "~" in item]
  configRegex = re.compile(r"\w+_config.xml")
  fileList = []
  for item in directoryList:
    theMatch = configRegex.match(item)
    if not theMatch == None:
      fileName = theMatch.group()
      fileRoot = fileName[0:fileName.find( "_config.xml" )]
      dataFileName = fileRoot + "_dat.xml"
      if not dataFileName in directoryList:
        fileList.append( fileRoot )
  return fileList
    
def GetDataFiles( subjectDirectory ):
  directoryList = [item for item in os.listdir( subjectDirectory ) if not "~" in item]
  configRegex = re.compile(r"\w+_dat.xml")
  fileList = []
  for item in directoryList:
    theMatch = configRegex.match(item)
    if not theMatch == None:
      fileDict = {}
      fileName = theMatch.group()
      fileDict['name'] = fileName
      fileRoot = fileName[0:fileName.find( "_dat.xml" )]
      textFileName = fileRoot + "_dat.txt"
      csvFileName = fileRoot + "_dat.csv"
      fileDict['converted'] = (textFileName in directoryList) or (csvFileName in directoryList)
      fileList.append( fileDict )
  return fileList

def BackupExptFile( fileName, dirPath, backupPath ):
  try:
    filePath = os.path.join( dirPath, fileName )
    s = os.stat( filePath )
    t = str( long(s.st_ctime ) )
    f = fileName.replace('=','_').replace('.','=')
    newFileName = f + '&%$' + t
    newFilePath = os.path.join( backupPath, newFileName )
    shutil.copyfile( filePath, newFilePath )
    return True
  except OSError:
    return False

def GetExptDirectories( exptPath ):
  try:
    dirList = os.listdir( exptPath )
  except OSError:
    print "GetExptDirectories error: os.listdir() failed."
    return []
  dirList = []
  for item in dirList:
    itemPath = os.path.join( exptPath, item )
    if os.path.isdir( itemPath ):
      if (item[-4:] == 'Data') and not (item[:4] == 'Demo'):
        dirList.append( item )
  return dirList
  
def GetExperimentList( exptPath ):
  try:
    dirList = os.listdir( exptPath )
  except OSError:
    print "GetExptDirectories error: os.listdir() failed."
    return []
  exptList = []
  for item in dirList:
    itemPath = os.path.join( exptPath, item )
    if os.path.isdir( itemPath ):
      if (item[-4:] == 'Data') and not (item[:4] == 'Demo'):
        exptList.append( item[:-4] )
  return exptList
  
"""
Testing stuff.
"""

expt1DataString = """<experiment_description>
  <experiment_name>LayoutTextureMemExpt1</experiment_name>
  <runtime>2003:36:51804/Thu Feb 06 14:23:24 2003</runtime>
  <comment>Pilot, sequential layout (no texture) response</comment>
  <factor_names>inspect_task|response_task|background|num_objects|inspect_sec_constant|inspect_sec_per_object|memory_secs|rep_num|session_num</factor_names>
  <factor_types>ARchar|ARchar|ARlong|ARlong|ARdouble|ARdouble|ARdouble|ARlong|ARlong</factor_types>
  <data_names>trial_start_secs|inspect_positions|inspect_textures|num_inspections|total_inspection_times|inspect_sequence|inspect_start_times|inspect_end_times|actual_memory_seconds|response_seconds|response_positions|num_times_placed</data_names>
  <data_types>ARdouble|ARfloat|ARint|ARint|ARdouble|ARint|ARdouble|ARdouble|ARdouble|ARdouble|ARdouble|ARint</data_types>
</experiment_description>

<subject_record_description>
  <field_names>label|name|eye_spacing_cm|eye_height_inch|age|gender</field_names>
  <field_types>ARchar|ARchar|ARfloat|ARfloat|ARint|ARchar</field_types>
</subject_record_description>

<subject_record>
  <age>38</age>
  <eye_height_inch>64.5</eye_height_inch>
  <eye_spacing_cm>5.8</eye_spacing_cm>
  <gender>M</gender>
  <label>SHMOO</label>
  <name>Jim Crowell</name>
</subject_record>

<trial_data>
  <inspect_task>look</inspect_task>
  <response_task>sequential_layout</response_task>
  <background>
    1 
  </background>
  <num_objects>
    8 
  </num_objects>
  <inspect_sec_constant>
    3 
  </inspect_sec_constant>
  <inspect_sec_per_object>
    2 
  </inspect_sec_per_object>
  <memory_secs>
    2 
  </memory_secs>
  <rep_num>
    1 
  </rep_num>
  <session_num>
    1 
  </session_num>
  <trial_start_secs>
    51.4279 
  </trial_start_secs>
  <inspect_positions>
    4.930000 5.378901 -2.592742 -4.930000 4.292914 -2.634474 4.930000 7.599676 
    -0.892083 4.930000 1.793451 -1.615152 -4.930000 5.041267 -0.201904 -2.815577 
    4.235445 -4.930000 4.676506 7.403333 -4.929999 -4.242790 7.672666 -4.930000 
      </inspect_positions>
  <inspect_textures>
    19 17 4 20 14 12 11 8 
      </inspect_textures>
  <num_inspections>
    3 2 1 1 1 3 3 2 
      </num_inspections>
  <total_inspection_times>
    4.02713 0.876231 0.918036 1.71095 0.730223 1.89858 1.14769 0.855632 
      </total_inspection_times>
  <inspect_sequence>
    6 8 2 5 2 8 6 7 
    1 3 1 4 7 1 7 6 
      </inspect_sequence>
  <inspect_start_times>
    0.736522 0.882622 0.966205 1.15395 1.88418 2.57268 3.34474 4.30453 
    4.86792 5.07664 5.99468 6.9128 9.35398 9.6461 12.5464 13.1307 
      </inspect_start_times>
  <inspect_end_times>
    0.882616 0.966198 1.15394 1.88417 2.57267 3.34474 3.92897 4.86791 
    5.07663 5.99467 6.9128 8.62376 9.6461 12.5464 12.8386 14.299 
      </inspect_end_times>
  <actual_memory_seconds>
    2.00629 
  </actual_memory_seconds>
  <response_seconds>
    16.2484 
  </response_seconds>
  <response_positions>
    4.93 3.12186 -1.94517 4.93 5.79815 -2.59346 4.93 7.28614 
    -4.03735 4.93 7.61023 -1.57795 -2.3423 5.1552 -4.93 -4.93 
    4.47083 -3.02115 -4.93 4.4564 -0.159206 -4.0753 7.94545 -4.93 
      </response_positions>
  <num_times_placed>
    2 1 1 1 1 1 1 1 
      </num_times_placed>
</trial_data>

<trial_data>
  <inspect_task>look</inspect_task>
  <response_task>sequential_layout</response_task>
  <background>
    1 
  </background>
  <num_objects>
    2 
  </num_objects>
  <inspect_sec_constant>
    3 
  </inspect_sec_constant>
  <inspect_sec_per_object>
    2 
  </inspect_sec_per_object>
  <memory_secs>
    2 
  </memory_secs>
  <rep_num>
    1 
  </rep_num>
  <session_num>
    1 
  </session_num>
  <trial_start_secs>
    92.7411 
  </trial_start_secs>
  <inspect_positions>
    4.930000 3.395529 0.582224 -1.940312 6.483284 -4.930000 
  </inspect_positions>
  <inspect_textures>
    1 2 
  </inspect_textures>
  <num_inspections>
    2 2 
  </num_inspections>
  <total_inspection_times>
    1.74418 1.37673 
  </total_inspection_times>
  <inspect_sequence>
    2 1 2 1 
  </inspect_sequence>
  <inspect_start_times>
    0.882502 2.25962 4.33797 5.46435 
  </inspect_start_times>
  <inspect_end_times>
    1.5711 4.0038 5.0261 7.01181 
  </inspect_end_times>
  <actual_memory_seconds>
    2.0233 
  </actual_memory_seconds>
  <response_seconds>
    6.63328 
  </response_seconds>
  <response_positions>
    4.93 3.58851 0.815652 -2.18098 6.47756 -4.93 
  </response_positions>
  <num_times_placed>
    2 1 
  </num_times_placed>
</trial_data>

<trial_data>
  <inspect_task>look</inspect_task>
  <response_task>sequential_layout</response_task>
  <background>
    1 
  </background>
  <num_objects>
    1 
  </num_objects>
  <inspect_sec_constant>
    3 
  </inspect_sec_constant>
  <inspect_sec_per_object>
    2 
  </inspect_sec_per_object>
  <memory_secs>
    2 
  </memory_secs>
  <rep_num>
    1 
  </rep_num>
  <session_num>
    1 
  </session_num>
  <trial_start_secs>
    112.521 
  </trial_start_secs>
  <inspect_positions>
    -4.930000 6.571779 -2.982708 
  </inspect_positions>
  <inspect_textures>
    20 
  </inspect_textures>
  <num_inspections>
    2 
  </num_inspections>
  <total_inspection_times>
    2.14913 
  </total_inspection_times>
  <inspect_sequence>
    1 1 
  </inspect_sequence>
  <inspect_start_times>
    0.791016 3.48249 
  </inspect_start_times>
  <inspect_end_times>
    2.94015 5.00874 
  </inspect_end_times>
  <actual_memory_seconds>
    2.02367 
  </actual_memory_seconds>
  <response_seconds>
    3.73314 
  </response_seconds>
  <response_positions>
    -4.93 6.54622 -3.21324 
  </response_positions>
  <num_times_placed>
    2 
  </num_times_placed>
</trial_data>
"""

expt2DataString = """<experiment_description>
  <experiment_name>ViewpointMemExpt1</experiment_name>
  <runtime>2003:152:35009/Mon Jun 02 09:43:29 2003</runtime>
  <comment>Test</comment>
  <factor_names>number_objects|inspect_seconds_constant|inspect_seconds_per_object|memory_seconds|repetition_number|session_number|test_view_angle|error_criterion|latency_criterion</factor_names>
  <factor_types>ARlong|ARdouble|ARdouble|ARdouble|ARlong|ARlong|ARdouble|ARdouble|ARdouble</factor_types>
  <data_names>trial_start_seconds|object_positions|object_textures|inspection_durations|training_response_order|training_response_positions|training_response_errors|training_response_latencies|test_response_order|test_response_positions|test_response_errors|test_response_latencies|walking_time</data_names>
  <data_types>ARdouble|ARfloat|ARint|ARdouble|ARint|ARfloat|ARfloat|ARdouble|ARint|ARfloat|ARfloat|ARdouble|ARdouble</data_types>
</experiment_description>

<subject_record_description>
  <field_names>label|name|eye_spacing_cm|age|gender</field_names>
  <field_types>ARchar|ARchar|ARfloat|ARint|ARchar</field_types>
</subject_record_description>

<subject_record>
  <age>38</age>
  <eye_spacing_cm>5.8</eye_spacing_cm>
  <gender>M</gender>
  <label>SHMOO</label>
  <name>Jim Crowell</name>
</subject_record>

<trial_data>
  <number_objects>
    2 
  </number_objects>
  <inspect_seconds_constant>
    3 
  </inspect_seconds_constant>
  <inspect_seconds_per_object>
    2 
  </inspect_seconds_per_object>
  <memory_seconds>
    5 
  </memory_seconds>
  <repetition_number>
    1 
  </repetition_number>
  <session_number>
    1 
  </session_number>
  <test_view_angle>
    -120 
  </test_view_angle>
  <error_criterion>
    1 
  </error_criterion>
  <latency_criterion>
    5 
  </latency_criterion>
  <trial_start_seconds>
    75.6056 
  </trial_start_seconds>
  <object_positions>
    1.569995 3.402346 0.192800 -1.284715 4.774839 -0.197374 
  </object_positions>
  <object_textures>
    19 4 
  </object_textures>
  <inspection_durations>
    7.02103 
  </inspection_durations>
  <training_response_order>
    0 1 
  </training_response_order>
  <training_response_positions>
    1.678820 3.638861 0.268037 -1.142339 5.130561 0.433186 
  </training_response_positions>
  <training_response_errors>
    0.271003 0.737845 
  </training_response_errors>
  <training_response_latencies>
    3.00112 2.31916 
  </training_response_latencies>
  <test_response_order>
    1 0 
  </test_response_order>
  <test_response_positions>
    1.853988 3.513507 0.664669 -0.232632 4.804233 0.727129 
  </test_response_positions>
  <test_response_errors>
    0.561845 1.400874 
  </test_response_errors>
  <test_response_latencies>
    2.83626 5.57741 
  </test_response_latencies>
  <walking_time>
    3.88473 
  </walking_time>
</trial_data>

<trial_data>
  <number_objects>
    4 
  </number_objects>
  <inspect_seconds_constant>
    3 
  </inspect_seconds_constant>
  <inspect_seconds_per_object>
    2 
  </inspect_seconds_per_object>
  <memory_seconds>
    5 
  </memory_seconds>
  <repetition_number>
    1 
  </repetition_number>
  <session_number>
    1 
  </session_number>
  <test_view_angle>
    0 
  </test_view_angle>
  <error_criterion>
    1 
  </error_criterion>
  <latency_criterion>
    5 
  </latency_criterion>
  <trial_start_seconds>
    138.164 
  </trial_start_seconds>
  <object_positions>
    -1.260378 3.718822 -0.262530 1.253609 4.454758 0.130185 -1.907147 4.862017 
    0.311931 -1.112134 3.243094 -0.380156 
  </object_positions>
  <object_textures>
    6 10 9 12 
  </object_textures>
  <inspection_durations>
    11.0047 
  </inspection_durations>
  <training_response_order>
    0 3 2 1 
  </training_response_order>
  <training_response_positions>
    -1.790235 4.017695 -0.027839 0.849735 5.030618 0.596461 -1.808554 5.004815 
    1.173940 -1.518802 3.620091 0.071011 
  </training_response_positions>
  <training_response_errors>
    0.652038 0.843885 0.879302 0.714883 
  </training_response_errors>
  <training_response_latencies>
    3.46427 2.3648 2.24413 3.86419 
  </training_response_latencies>
  <test_response_order>
    3 0 2 1 
  </test_response_order>
  <test_response_positions>
    -1.883438 4.142591 -0.237895 1.050107 4.628899 0.015322 -2.409590 4.904053 
    0.588280 -1.660280 3.723615 -0.250899 
  </test_response_positions>
  <test_response_errors>
    0.753918 0.291430 0.574965 0.740319 
  </test_response_errors>
  <test_response_latencies>
    4.98906 2.9447 2.23458 5.41713 
  </test_response_latencies>
  <walking_time>
    5 
  </walking_time>
</trial_data>
"""

expt2ConfigString = """<experiment_description>
  <experiment_name>ViewpointMemExpt1</experiment_name>
  <method>enumerated</method>
  <subject>SHMOO</subject>
  <num_trials>8</num_trials>
  <comment>Test</comment>
  <factor_names>number_objects|inspect_seconds_constant|inspect_seconds_per_object|memory_seconds|repetition_number|session_number|test_view_angle|error_criterion|latency_criterion</factor_names>
  <factor_types>ARlong|ARdouble|ARdouble|ARdouble|ARlong|ARlong|ARdouble|ARdouble|ARdouble</factor_types>
</experiment_description>

<trial_record>
  <number_objects>2</number_objects>
  <inspect_seconds_constant>3</inspect_seconds_constant>
  <inspect_seconds_per_object>2</inspect_seconds_per_object>
  <memory_seconds>5</memory_seconds>
  <error_criterion>1</error_criterion>
  <latency_criterion>5</latency_criterion>
  <test_view_angle>-120</test_view_angle>
  <repetition_number>1</repetition_number>
  <session_number>1</session_number>
</trial_record>

<trial_record>
  <number_objects>4</number_objects>
  <inspect_seconds_constant>3</inspect_seconds_constant>
  <inspect_seconds_per_object>2</inspect_seconds_per_object>
  <memory_seconds>5</memory_seconds>
  <error_criterion>1</error_criterion>
  <latency_criterion>5</latency_criterion>
  <test_view_angle>0</test_view_angle>
  <repetition_number>1</repetition_number>
  <session_number>1</session_number>
</trial_record>

<trial_record>
  <number_objects>2</number_objects>
  <inspect_seconds_constant>3</inspect_seconds_constant>
  <inspect_seconds_per_object>2</inspect_seconds_per_object>
  <memory_seconds>5</memory_seconds>
  <error_criterion>1</error_criterion>
  <latency_criterion>5</latency_criterion>
  <test_view_angle>-120</test_view_angle>
  <repetition_number>1</repetition_number>
  <session_number>1</session_number>
</trial_record>
"""

if __name__ == '__main__':
  print GetExperimentList( r"G:\Experiments" )
#  s = ReadFile(r"G:\Experiments\ViewpointMemExpt1Data\SHMOO\test_dat.xml")
#  d = ParseXMLDataFile( s )
#  print d
  #print ParseXMLDataFile( expt2DataString )
  #print ParseDataFile( expt2DataString )
  #d = ParseXMLConfigFile( expt2ConfigString )
  #d2 = ParseXMLConfigFile( MakeXMLConfigFile( d ) )
  #print d == d2
  #f = file(r"G:\Experiments\ViewpointMemExpt1Data\subject_data.xml","r")
  #s1 = f.read()
  #f.close()
  #print ParseSubjectDatabase( s1 )
  #l1 = ParseSubjectInfoString(s1)
  #l2 = ParseSubjectInfoString( MakeSubjectInfoString( l1 ) )
  #print l1 == l2
#  print GetDataFiles( r"G:\Experiments\LayoutTextureMemExpt1Data\SHMOO" )
