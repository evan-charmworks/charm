/***************************************************************************
 * RCS INFORMATION:
 *
 *	$RCSfile$
 *	$Author$	$Locker$		$State$
 *	$Revision$	$Date$
 *
 ***************************************************************************
 * DESCRIPTION:
 *
 ***************************************************************************
 * REVISION HISTORY:
 *
 * $Log$
 * Revision 2.14  1995-09-06 04:08:58  sanjeev
 * fixed bugs
 *
 * Revision 2.13  1995/09/05  22:00:52  sanjeev
 * modified StartCharm and ProcessBocInit to integrate Charm++.
 *
 * Revision 2.12  1995/09/01  02:13:17  jyelon
 * VID_BLOCK, CHARE_BLOCK, BOC_BLOCK consolidated.
 *
 * Revision 2.11  1995/07/27  20:29:34  jyelon
 * Improvements to runtime system, general cleanup.
 *
 * Revision 2.10  1995/07/24  01:54:40  jyelon
 * *** empty log message ***
 *
 * Revision 2.9  1995/07/22  23:44:13  jyelon
 * *** empty log message ***
 *
 * Revision 2.8  1995/07/19  22:15:26  jyelon
 * *** empty log message ***
 *
 * Revision 2.7  1995/07/12  16:28:45  jyelon
 * *** empty log message ***
 *
 * Revision 2.6  1995/06/29  21:43:17  narain
 * Added CldCreateBoc
 *
 * Revision 2.5  1995/06/29  15:54:09  gursoy
 * fixed a CpvAccess -- ReadBufIndex
 *
 * Revision 2.4  1995/06/20  14:54:24  gursoy
 * fixed readonly's
 *
 * Revision 2.3  1995/06/18  22:10:34  sanjeev
 * removed CondSendInit
 *
 * Revision 2.2  1995/06/13  14:33:55  gursoy
 * *** empty log message ***
 *
 * Revision 1.16  1995/05/03  20:56:44  sanjeev
 * bug fixes for finding uninitialized modules
 *
 * Revision 1.15  1995/05/03  06:28:20  sanjeev
 * registered _CkNullFunc
 *
 * Revision 1.14  1995/05/02  20:37:46  milind
 * Added _CkNullFunc()
 *
 * Revision 1.13  1995/04/25  03:40:01  sanjeev
 * fixed dynamic boc creation bug in ProcessBocInitMsg
 *
 * Revision 1.12  1995/04/23  20:53:14  sanjeev
 * Removed Core....
 *
 * Revision 1.11  1995/04/13  20:54:18  sanjeev
 * Changed Mc to Cmi
 *
 * Revision 1.10  1995/04/06  17:46:21  sanjeev
 * fixed bug in tracing Charm++ BranchInits
 *
 * Revision 1.9  1995/04/02  00:48:57  sanjeev
 * changes for separating Converse
 *
 * Revision 1.8  1995/03/25  18:25:40  sanjeev
 * *** empty log message ***
 *
 * Revision 1.7  1995/03/24  16:42:59  sanjeev
 * *** empty log message ***
 *
 * Revision 1.6  1995/03/21  20:55:23  sanjeev
 * Changes for new converse names
 *
 * Revision 1.5  1995/03/17  23:38:27  sanjeev
 * changes for better message format
 *
 * Revision 1.4  1995/03/12  17:10:39  sanjeev
 * changes for new msg macros
 *
 * Revision 1.3  1994/12/02  00:02:26  sanjeev
 * interop stuff
 *
 * Revision 1.1  1994/11/03  17:39:52  brunner
 * Initial revision
 *
 ***************************************************************************/
static char     ident[] = "@(#)$Header$";
/***************************************************************************/
/* This is the main process that runs on each pe                           */
/* */
/***************************************************************************/
#include "chare.h"
#include "globals.h"
#include "performance.h"


/* these two variables are set in registerMainChare() */
CsvDeclare(int, _CK_MainChareIndex);
CsvDeclare(int, _CK_MainEpIndex);

CsvDeclare(int, ReadBuffSize); /* this is set in registerReadOnly() */
CsvStaticDeclare(void, **_CK_9_ReadMsgTable);	/* was previously global */

CpvDeclare(char*, ReadBufIndex);
CpvDeclare(char*, ReadFromBuffer);

CsvExtern(FUNCTION_PTR*,  ChareFnTable);

#define BLK_LEN 512
CpvStaticDeclare(BOCINIT_QUEUE*, BocInitQueueHead);


void *BocInitQueueCreate() ;
ENVELOPE *DeQueueBocInitMsgs() ;

/* these store argc, argv for use by CharmInit */
CpvStaticDeclare(int,   userArgc);
CpvStaticDeclare(char**,  userArgv);


/* all these counts are incremented in register.c */
CpvExtern(int,      fnCount);
CpvExtern(int,      msgCount);
CpvExtern(int,      chareCount);
CpvExtern(int,      chareEpsCount);	/* count of chare AND boc eps */
CpvExtern(int,      pseudoCount);
CpvExtern(int,      readMsgCount);
CpvExtern(int,      readCount);	/* count of read-only functions. Should be
				 * same as total number of modules */
CsvStaticDeclare(int, sharedReadCount); /* it holds the value of readCount, */
                                        /* and it is  shared within node    */

extern void CPlus_ProcessBocInitMsg();	/* in cplus_node_init.c */
extern void CPlus_CallCharmInit();	/* in cplus_node_init.c */
extern void CPlus_SetMainChareID();	/* in cplus_node_init.c */
extern CHARE_BLOCK *CreateChareBlock();


extern void HANDLE_INCOMING_MSG() ;
extern void CkProcess_ForChareMsg();
extern void CkProcess_DynamicBocInitMsg();
extern void CkProcess_NewChareMsg();
extern void CkProcess_BocMsg();
extern void CkProcess_VidSendOverMsg();

void initModuleInit()
{
    CpvInitialize(char*, ReadBufIndex);
    CpvInitialize(char*, ReadFromBuffer);
    CpvInitialize(BOCINIT_QUEUE*, BocInitQueueHead);
    CpvInitialize(int, userArgc);
    CpvInitialize(char**, userArgv);
    if (CmiMyRank() == 0) CsvAccess(ReadBuffSize) = 0;
}




/*Added By Milind 05/02/95 */

void _CkNullFunc()
{
	CmiPrintf("[%d] In Null Function: Module Uninitialized\n", CmiMyPe());
}




static int CountArgs(argv)
char **argv;
{
    int argc=0;
    while (*argv) { argc++; argv++; }
    return argc;
}


StartCharm(argv)
char **argv;
{
	int             i;
	char           *ReadBufMsg;

        CpvAccess(userArgc) = ParseCommandOptions(CountArgs(argv), argv);
        CpvAccess(userArgv) = argv;

	InitializeMessageMacros();

	CmiSpanTreeInit();

        /* OtherQsInit(); this was combined with CsdInitialize */
        StatInit();
        InitializeDynamicBocMsgList();
        InitializeBocDataTable();
        InitializeBocIDMessageCountTable();
        
        if (CmiMyRank() == 0) InitializeEPTables();
        CmiNodeBarrier();          
  
 
        log_init();
        trace_begin_computation();
        SysBocInit();
        CpvAccess(msgs_created) = CpvAccess(msgs_processed) = 0;


	if (CmiMyPe() == 0)
	{
		CpvAccess(MsgCount) = 0; 
                                 /* count # of messages being sent to each
				 * node. assume an equal number gets sent to
				 * every one. if there is a difference, have
				 * to modify this somewhat */
		CpvAccess(InsideDataInit) = 1;

		CldCreateBoc();

		CpvAccess(InsideDataInit) = 0;

		trace_begin_charminit();
		 
		CpvAccess(MainDataSize) = CsvAccess(ChareSizesTable)
					      [CsvAccess(_CK_MainChareIndex)];
		CpvAccess(mainChareBlock) = CpvAccess(currentChareBlock) = 
					(CHARE_BLOCK *)CreateChareBlock(
						CpvAccess(MainDataSize),
						CHAREKIND_CHARE, rand());

		if (CsvAccess(MainChareLanguage) != CHARMPLUSPLUS) 
			mainChareBlock->chareptr = mainChareBlock + 1 ;
		else {
			mainChareBlock->chareptr = (void *)
						((CsvAccess(ChareFnTable)
					      [CsvAccess(_CK_MainChareIndex)])
						(CpvAccess(mainChareBlock)));
			CPlus_SetMainChareID() ;  /* set mainhandle */
		}


		/* Calling CharmInit entry point */
		CpvAccess(NumReadMsg) = 0;
		CpvAccess(InsideDataInit) = 1;

		(CsvAccess(EpInfoTable)[CsvAccess(_CK_MainEpIndex)].function)
		  		(NULL, CpvAccess(currentChareBlock)->chareptr,
				   CpvAccess(userArgc), CpvAccess(userArgv));
		
		CpvAccess(InsideDataInit) = 0;
		trace_end_charminit();

		/* create the buffer for the read only variables */
		ReadBufMsg = (char *) CkAllocMsg(CsvAccess(ReadBuffSize));
		CpvAccess(ReadBufIndex) = ReadBufMsg;
		if (CsvAccess(ReadBuffSize) > 0)
			CkMemError(ReadBufMsg);

		/*
		 * in Charm++ the CopyToBuffer fns also send out the
		 * ReadonlyMsgs by calling ReadMsgInit()
		 */
		for (i = 0; i < CsvAccess(sharedReadCount); i++)
			(CsvAccess(ROCopyToBufferTable)[i]) ();

		/*
		 * we are sending the id of the main chare along with the
		 * read only message. in future versions, we might eliminate
		 * this because the functionality can be expressed using
		 * readonly variables and MyChareID inside the main chare
		 */

		BroadcastReadBuffer(ReadBufMsg, CsvAccess(ReadBuffSize), CpvAccess(mainChareBlock));

		/*
		 * send a message with the count of initial messages sent so
		 * far, to all nodes; includes messages for read-buffer and
		 * bocs
		 */
		BroadcastCount();
	}
	else
	{
		CharmInitLoop();
	}
	SysPeriodicCheckInit();

	/* Loop(); 	- Narain 11/16 */
}


/*
 * Receive read only variable buffer, read only messages and  BocInit
 * messages. Start Boc's by allocating and filling the NodeBocTbl. Wait until
 * (a) the  "count" message is received and (b) "count" number of initial
 * messages are received.
 */
CharmInitLoop()
{
  int             i, id, type;
  void           *usrMsg;
  int             countInit = 0;
  extern void    *CmiGetMsg();
  int         countArrived = 0;
  ENVELOPE       *envelope, *readvarmsg;
  
  CpvAccess(BocInitQueueHead) = (BOCINIT_QUEUE *) BocInitQueueCreate();
  
  while ((!countArrived) || (countInit != 0))
    {
      envelope = NULL;
      while (envelope == NULL)
	envelope = (ENVELOPE *) CmiGetMsg();
      if ((GetEnv_msgType(envelope) == BocInitMsg) ||
	  (GetEnv_msgType(envelope) == ReadMsgMsg))
	UNPACK(envelope);
      usrMsg = USER_MSG_PTR(envelope);
      /* Have a valid message now. */
      type = GetEnv_msgType(envelope);
      switch (type)
	{
	  
	case BocInitMsg:
	  EnQueueBocInitMsgs(envelope);
	  countInit++;
	  break;
	  
	case InitCountMsg:
	  countArrived = 1;
	  countInit -= GetEnv_count(envelope);
	  CmiFree(envelope);
	  break;
	  
	case ReadMsgMsg:
	  id = GetEnv_other_id(envelope);
	  CsvAccess(_CK_9_ReadMsgTable)[id] = (void *) usrMsg;
	  countInit++;
	  break;
	  
	case ReadVarMsg:
	  CpvAccess(ReadFromBuffer) = usrMsg;
	  countInit++;
	  
	  /* get the information about the main chare */
	  CpvAccess(mainChareBlock) = (struct chare_block *)
	    GetEnv_chareBlockPtr(envelope);
	  CpvAccess(mainChare_magic_number) =
	    GetEnv_chare_magic_number(envelope);
	  if (CsvAccess(MainChareLanguage) == CHARMPLUSPLUS)
	    CPlus_SetMainChareID();
	  readvarmsg = envelope;
	  break;
	  
	case LdbMsg:
	case BocMsg:
	case ImmBocMsg:
	case QdBocMsg:
	case BroadcastBocMsg:
	case ImmBroadcastBocMsg:
	case QdBroadcastBocMsg:      
	  CmiSetHandler(envelope,CsvAccess(CkProcIdx_BocMsg));
	  CkEnqueue(envelope);
	  break;

	case NewChareNoBalanceMsg:
	case NewChareMsg:
	  CmiSetHandler(envelope,CsvAccess(CkProcIdx_NewChareMsg));
	  CkEnqueue(envelope);
	  break;
	  
	case ForChareMsg:
	  CmiSetHandler(envelope,CsvAccess(CkProcIdx_ForChareMsg));
	  CkEnqueue(envelope);
	  break;
	  
	case DynamicBocInitMsg:
	  CmiSetHandler(envelope,CsvAccess(CkProcIdx_DynamicBocInitMsg));
	  CkEnqueue(envelope);
	  break;
	  
	case VidSendOverMsg:
	  CmiSetHandler(envelope,CsvAccess(CkProcIdx_VidSendOverMsg));
	  CkEnqueue(envelope);
	  break;
	  
        default:
          CmiPrintf("** ERROR ** Unknown message type %d\n",type);
	}
    }

  /*
   * call all the CopyFromBuffer functions for ReadOnly variables.
   * _CK_9_ReadMsgTable is passed as an arg because it is no longer
   * global
   */
  if (CmiMyRank() == 0) {
    for (i = 0; i < CsvAccess(sharedReadCount); i++)
      (CsvAccess(ROCopyFromBufferTable)[i]) (CsvAccess(_CK_9_ReadMsgTable));
  }
  CmiFree(readvarmsg);
  
  while ( (envelope=DeQueueBocInitMsgs()) != NULL ) 
    ProcessBocInitMsg(envelope);
}

ProcessBocInitMsg(envelope)
ENVELOPE       *envelope;
{
  CHARE_BLOCK    *bocBlock;
  void           *usrMsg = USER_MSG_PTR(envelope);
  int             current_ep = GetEnv_EP(envelope);
  EP_STRUCT      *current_epinfo = CsvAccess(EpInfoTable) + current_ep;
  int             current_bocnum = GetEnv_boc_num(envelope);
  int             current_msgType = GetEnv_msgType(envelope);
  int             current_chare = current_epinfo->chareindex;
  int             current_magic = rand();

  bocBlock = CreateChareBlock(CsvAccess(ChareSizesTable)[current_chare], 
					CHAREKIND_BOCNODE, current_magic);
  bocBlock->x.boc_num = current_bocnum;
  if ( current_epinfo->language != CHARMPLUSPLUS ) 
    bocBlock->chareptr = (void *) (bocBlock + 1) ; 
  else
    bocBlock->chareptr = (void *)((CsvAccess(ChareFnTable)[current_chare])
						(bocBlock)) ;
  SetBocDataPtr(current_bocnum, bocBlock->chareptr);
  trace_begin_execute(envelope);
  (current_epinfo->function)(usrMsg, bocBlock->chareptr);
  trace_end_execute(current_bocnum, current_msgType, current_ep);

  /* for dynamic BOC creation, used in node_main.c */
  return current_bocnum ;
}


/* this call can only be made after the clock has been initialized */

SysPeriodicCheckInit()
{
	CldPeriodicCheckInit();
}


int 
ParseCommandOptions(argc, argv)
int             argc;
char          **argv;
{
/* Removed Converse options into ConverseParseCommandOptions. - Sanjeev */
	/*
	 * configure the chare kernel according to command line parameters.
	 * by convention, chare kernel parameters begin with '+'.
	 */
	int             i, j, numSysOpts = 0, foundSysOpt = 0, end;
	int             mainflag = 0, memflag = 0;
        int             numPe;
	if (argc < 1)
	{
		CmiPrintf("Too few arguments. Usage> host_prog node_prog [...]\n");
		exit(1);
	}

	end = argc;
	if (CmiMyPe() == 0)
		mainflag = 1;

	for (i = 1; i < end; i++)
	{
		foundSysOpt = 0;
		if (strcmp(argv[i], "+cs") == 0)
		{
			CpvAccess(PrintChareStat) = 1;
			/*
			 * if (mainflag) CmiPrintf("Chare Statistics Turned
			 * On\n");
			 */
			foundSysOpt = 1;
		}
		else if (strcmp(argv[i], "+ss") == 0)
		{
			CpvAccess(PrintSummaryStat) = 1;
			/*
			 * if(mainflag)CmiPrintf("Summary Statistics Turned
			 * On\n");
			 */
			foundSysOpt = 1;
		}
                else if (strcmp(argv[i], "+p") == 0 && i + 1 < argc)
                {
                        sscanf(argv[i + 1], "%d", &numPe);
                        foundSysOpt = 2;
                }
                else if (sscanf(argv[i], "+p%d", &numPe) == 1)
                {
                        foundSysOpt = 1;
                }
		if (foundSysOpt)
		{
			/* if system option, remove it. */
			numSysOpts += foundSysOpt;
			end -= foundSysOpt;
			for (j = i; j < argc - foundSysOpt; j++)
			{
				argv[j] = argv[j + foundSysOpt];
			}
			/* reset i because we shuffled everything down one */
			i--;
		}

	}
	return (argc - numSysOpts);
}



#define TABLE_SIZE 256

InitializeEPTables()
{
  int             i;
  int             TotalFns;
  int             TotalMsgs;
  int             TotalChares;
  int             TotalModules;
  int             TotalReadMsgs;
  int             TotalPseudos;
  EP_STRUCT      *epinfo;
  
  /*
   * TotalEps 	=  _CK_5mainChareEPCount(); TotalFns	=
   * _CK_5mainFunctionCount(); TotalMsgs	=  _CK_5mainMessageCount();
   * TotalChares 	=  _CK_5mainChareCount(); TotalBocEps 	=
   * NumSysBocEps + _CK_5mainBranchEPCount();
   */
  TotalEps = TABLE_SIZE;
  TotalFns = TABLE_SIZE;
  TotalMsgs = TABLE_SIZE;
  TotalChares = TABLE_SIZE;
  TotalModules = TABLE_SIZE;
  TotalReadMsgs = TABLE_SIZE;
  TotalPseudos = TABLE_SIZE;
  
  /*
   * this table is used to store all ReadOnly Messages on processors
   * other than proc 0. After they are received, they are put in the
   * actual variables in the user program in the ...CopyFromBuffer
   * functions
   */
  CsvAccess(_CK_9_ReadMsgTable) = (void **) 
    CmiSvAlloc((TotalReadMsgs + 1) * sizeof(void *));
  if (TotalReadMsgs > 0)
    CkMemError(CsvAccess(_CK_9_ReadMsgTable));
  
  CsvAccess(ROCopyFromBufferTable) = (FUNCTION_PTR *) 
    CmiSvAlloc((TotalModules + 1) * sizeof(FUNCTION_PTR));
  
  CsvAccess(ROCopyToBufferTable) = (FUNCTION_PTR *) 
    CmiSvAlloc((TotalModules + 1) * sizeof(FUNCTION_PTR));
  
  if (TotalModules > 0)
    {
      CkMemError(CsvAccess(ROCopyFromBufferTable));
      CkMemError(CsvAccess(ROCopyToBufferTable));
    }
  
  epinfo=(EP_STRUCT*)CmiSvAlloc((TotalEps+1)*sizeof(EP_STRUCT));
  CsvAccess(EpInfoTable)=epinfo;
  if (TotalEps > 0) {
    CkMemError(epinfo);
    memset((char *)epinfo, 0, (TotalEps+1)*sizeof(EP_STRUCT));
    for (i = 0; i < CpvAccess(chareEpsCount); i++)
      epinfo[i].language = -1;
  }
  
  _CK_9_GlobalFunctionTable = (FUNCTION_PTR *) 
    CmiSvAlloc((TotalFns + 1) * sizeof(FUNCTION_PTR));
  
  if (TotalFns > 0)
    CkMemError(_CK_9_GlobalFunctionTable);
  
  
  CsvAccess(MsgToStructTable) = (MSG_STRUCT *) 
    CmiSvAlloc((TotalMsgs + 1) * sizeof(MSG_STRUCT));
  
  if (TotalMsgs > 0)
    CkMemError(CsvAccess(MsgToStructTable));
  
  
  CsvAccess(ChareSizesTable) = (int *) 
    CmiSvAlloc((TotalChares + 1) * sizeof(int));
  
  CsvAccess(ChareNamesTable) = (char **) CmiSvAlloc(TotalChares * sizeof(char *));
  CsvAccess(ChareFnTable) = (FUNCTION_PTR *) 
    CmiSvAlloc((TotalChares + 1) * sizeof(FUNCTION_PTR));
  
  if (TotalChares > 0)
    {
      CkMemError(CsvAccess(ChareSizesTable));
      CkMemError(CsvAccess(ChareNamesTable));
      CkMemError(CsvAccess(ChareFnTable));
    }
  
  CsvAccess(PseudoTable) = (PSEUDO_STRUCT *) 
    CmiSvAlloc((TotalPseudos + 1) * sizeof(PSEUDO_STRUCT));
  
  if (TotalPseudos > 0)
    CkMemError(CsvAccess(PseudoTable));
  
  
  
  /** end of table allocation **/
  
  /* Register the NullFunction to detect uninitialized modules */
  registerMsg("NULLMSG",_CkNullFunc,_CkNullFunc,_CkNullFunc,0) ;
  registerEp("NULLEP",_CkNullFunc,0,0,0) ;
  registerChare("NULLCHARE",0,_CkNullFunc) ;
  registerFunction(_CkNullFunc) ;
  registerMonotonic("NULLMONO",_CkNullFunc,_CkNullFunc,CHARM) ;
  registerTable("NULLTABLE",_CkNullFunc,_CkNullFunc) ;
  registerAccumulator("NULLACC",_CkNullFunc,_CkNullFunc,_CkNullFunc,CHARM) ;
  
  /* Register all the built-in BOC's */
  AddSysBocEps();
  NumSysBocEps = chareEpsCount;

  /*
   * This is the top level call to all modules for initialization. It
   * is generated at link time by charmc, in module_init_fn.c
   */
  _CK_module_init_fn();
  
  if ( CsvAccess(MainChareLanguage) == -1 ) {
    CmiPrintf("[%d] ERROR: registerMainChare() not called : uninitialized module exists\n",CmiMyPe()) ;
  }
  
  /* Register the Charm handlers with Converse */
  CsvAccess(HANDLE_INCOMING_MSG_Index)
    = CmiRegisterHandler(HANDLE_INCOMING_MSG) ;
  CsvAccess(CkProcIdx_ForChareMsg)
    = CmiRegisterHandler(CkProcess_ForChareMsg);
  CsvAccess(CkProcIdx_DynamicBocInitMsg)
    = CmiRegisterHandler(CkProcess_DynamicBocInitMsg);
  CsvAccess(CkProcIdx_NewChareMsg)
    = CmiRegisterHandler(CkProcess_NewChareMsg);
  CsvAccess(CkProcIdx_BocMsg)
    = CmiRegisterHandler(CkProcess_BocMsg);
  CsvAccess(CkProcIdx_VidSendOverMsg)
    = CmiRegisterHandler(CkProcess_VidSendOverMsg);
  
  /* set all the "Total" variables so that the rest of the modules work */
  TotalEps = CpvAccess(chareEpsCount);
  TotalFns = CpvAccess(fnCount);
  TotalMsgs = CpvAccess(msgCount);
  TotalChares = CpvAccess(chareCount);
  TotalModules = CpvAccess(readCount);
  TotalReadMsgs = CpvAccess(readMsgCount);
  TotalPseudos = CpvAccess(pseudoCount);
  
  CsvAccess(sharedReadCount) = CpvAccess(readCount);
}

/* Adding entry points for system branch office chares. */
AddSysBocEps()
{
	CldAddSysBocEps();
	QDAddSysBocEps();
	WOVAddSysBocEps();
	TblAddSysBocEps();
	AccAddSysBocEps();
	MonoAddSysBocEps();
	DynamicAddSysBocEps();
	StatAddSysBocEps();
}


/* Broadcast the count of messages that are received during initialization. */
BroadcastCount()
{
	ENVELOPE       *env;
	void           *dummy_msg;
	dummy_msg = (int *) CkAllocMsg(sizeof(int));
	CkMemError(dummy_msg);
	env = ENVELOPE_UPTR(dummy_msg);
	SetEnv_msgType(env, InitCountMsg);

	SetEnv_count(env, CpvAccess(currentBocNum) - NumSysBoc + 2 + CpvAccess(NumReadMsg));

	CkCheck_and_Broadcast(env);
	/* CkFreeMsg(dummy_msg);  commented on Jun 23 */
}

int 
EmptyBocInitMsgs()
{
	return (CpvAccess(BocInitQueueHead)->length == 0);
}


void           *
BocInitQueueCreate()
{
	BOCINIT_QUEUE  *queue;
	queue = (BOCINIT_QUEUE *) CmiAlloc(sizeof(BOCINIT_QUEUE));
	queue->block = (void **) CmiAlloc(sizeof(void *) * BLK_LEN);
	queue->block_len = BLK_LEN;
	queue->first = queue->block_len;
	queue->length = 0;
	return (void *) queue;
}


EnQueueBocInitMsgs(envelope)
ENVELOPE       *envelope;
{
	int             num = GetEnv_boc_num(envelope);
	int i ;

	if (num > CpvAccess(BocInitQueueHead)->block_len)
	{
		void          **blk = CpvAccess(BocInitQueueHead)->block;
		int             last;
		CpvAccess(BocInitQueueHead)->block = (void **) CmiAlloc(sizeof(void *) * (num + BLK_LEN));
		last = CpvAccess(BocInitQueueHead)->first + CpvAccess(BocInitQueueHead)->length;
		for (i = CpvAccess(BocInitQueueHead)->first; i < last; i++)
			CpvAccess(BocInitQueueHead)->block[i] = blk[i];
		CpvAccess(BocInitQueueHead)->block[num] = envelope;
		CpvAccess(BocInitQueueHead)->length++;
		CmiFree(blk);
	}
	else
	{
		CpvAccess(BocInitQueueHead)->block[num] = envelope;
		CpvAccess(BocInitQueueHead)->length++;
		if (CpvAccess(BocInitQueueHead)->first > num)
			CpvAccess(BocInitQueueHead)->first = num;
	}
}


ENVELOPE *DeQueueBocInitMsgs()
{
	ENVELOPE      *envelope;
	if (CpvAccess(BocInitQueueHead)->length)
	{
		envelope = CpvAccess(BocInitQueueHead)->block[CpvAccess(BocInitQueueHead)->first++];
		CpvAccess(BocInitQueueHead)->length--;
	/*	if (!CpvAccess(BocInitQueueHead)->length)
			CpvAccess(BocInitQueueHead)->first = CpvAccess(BocInitQueueHead)->block_len;
	*/
		return envelope ;
	}
	else
		return NULL ;
}

SysBocInit()
{
	QDBocInit();
	TblBocInit();
	WOVBocInit();
	DynamicBocInit();
	StatisticBocInit();
}



hostep_error(msg, mydata)
void           *msg, *mydata;
{
	CmiPrintf("****error*** main chare ep called on node %d.\n",
		 CmiMyPe());
}
